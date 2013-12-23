/*
 * The main lbcd server functionality.
 *
 * Handles parsing command-line options, setting up the daemon, waiting for
 * packets, and handling each incoming request.
 *
 * Written by Larry Schwimmer
 * Extensively modified by Russ Allbery <eagle@eyrie.org>
 * Copyright 1996, 1997, 1998, 2005, 2006, 2008, 2012, 2013
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/sd-daemon.h>
#include <portable/socket.h>
#include <portable/system.h>

#include <errno.h>
#include <signal.h>
#include <syslog.h>

#include <lbcd/internal.h>
#include <util/messages.h>
#include <util/network.h>
#include <util/vector.h>
#include <util/xmalloc.h>

/* The usage message. */
const char usage_message[] = "\
Usage: lbcd [options] [-d] [-p <port>]\n\
   -b <addr>    bind to <addr> instead of all available addresses\n\
   -c <cmd>     run <cmd> (full path) to obtain load values\n\
   -d           debug mode, don't fork or log to syslog\n\
   -f           run in the foreground\n\
   -h, --help   print usage\n\
   -l           log various requests\n\
   -P <file>    write PID to <file>\n\
   -p <port>    run using different port number\n\
   -R           round-robin polling\n\
   -S           don't adjust version two responses for custom services\n\
   -T <seconds> timeout (1-300 seconds, default 5)\n\
   -t           test mode (print stats and exit)\n\
   -w <option>  specify returned weight; options:\n\
                  either \"load:incr\" or \"service\"\n\
   --version    print protocol version and exit\n\
   -Z           raise SIGSTOP once ready to answer queries\n";

/* Stores configuration information for lbcd. */
struct lbcd_config {
    struct vector *bindaddrs;   /* Address to listen on */
    bool log;                   /* Log each request */
    unsigned short port;        /* Port to listen on */
    const char *pid_file;       /* Write the daemon PID to this path */
    struct vector *services;    /* Allowed services */
    bool simple;                /* Do not adjust results for version 2 */
    bool upstart;               /* Raise SIGSTOP when ready for upstart */
};

/*
 * Stores relevant information about a client request.  Do not confuse with an
 * lbcd_request, which is the wire representation of a protocol request.
 */
struct request {
    struct sockaddr *addr;      /* Address of client */
    socklen_t addrlen;          /* Length of client address */
    char *source;               /* String representation of client address */
    unsigned int protocol;      /* Protocol version of request */
    unsigned int id;            /* Client-provided request ID */
    unsigned int operation;     /* Requested lbcd operation */
    struct vector *services;    /* Requested services */
};


/*
 * Print out the usage message and then exit with the status given as the
 * only argument.  If status is zero, the message is printed to standard
 * output; otherwise, it is sent to standard error.
 */
static void
usage(int status)
{
    fprintf((status == 0) ? stdout : stderr, "%s", usage_message);
    exit(status);
}


/*
 * Print out version information and exit successfully.
 */
static void
version(void)
{
    printf("lbcd protocol %d version %s\n", LBCD_VERSION, PACKAGE_VERSION);
    exit(0);
}


/*
 * Free the request information.
 */
static void
request_free(struct request *request)
{
    if (request == NULL)
        return;
    free(request->addr);
    free(request->source);
    vector_free(request->services);
    free(request);
}


/*
 * Send a status reply back to the client.  Takes the client information,
 * socket to use to send messages, and client status.
 */
static void
send_status(struct request *request, socket_type fd, enum lbcd_status status)
{
    struct lbcd_header header;
    size_t size;
    ssize_t result;

    /* Build the response packet. */
    header.version = htons(LBCD_VERSION);
    header.id      = htons(request->id);
    header.op      = htons(request->operation);
    header.status  = htons(status);

    /* Send the packet to the client. */
    size = sizeof(header);
    result = sendto(fd, &header, size, 0, request->addr, request->addrlen);
    if (result != (ssize_t) size)
        syswarn("client %s: cannot send reply", request->source);
}


/*
 * Given a service name, check whether it's in the allowed list or is one of
 * the special allowed services.  Returns true if it's allowed and false
 * otherwise.
 */
static bool
service_allowed(struct lbcd_config *config, const char *service)
{
    size_t i;

    /* The default service is always allowed. */
    if (strcmp(service, "default") == 0)
        return true;

    /* The cmd service is never allowed, even if configured. */
    if (strcmp(service, "cmd") == 0 || strncmp(service, "cmd:", 4) == 0)
        return false;

    /* Otherwise, check if it's in the allowed list. */
    for (i = 0; i < config->services->count; i++)
        if (strcmp(config->services->strings[i], service) == 0)
            return true;
    return false;
}


/*
 * Receive request packet and verify the integrity and format of the reply.
 * This routine is REQUIRED to sanitize the request packet.  All other program
 * routines can expect that the packet is safe to read once it is passed on.
 *
 * Returns a newly-allocated lbcd_request struct on success and NULL on
 * failure.
 */
static struct request *
request_recv(struct lbcd_config *config, socket_type fd)
{
    struct sockaddr_storage addr;
    struct sockaddr *sockaddr;
    socklen_t addrlen;
    ssize_t result;
    char raw[LBCD_MAXMESG];
    char source[INET6_ADDRSTRLEN] = "UNKNOWN";
    struct lbcd_request *packet;
    unsigned int protocol, id, operation, nservices, i;
    size_t expected;
    struct request *request;
    char *service;

    /* Receive the UDP packet from the wire. */
    addrlen = sizeof(addr);
    sockaddr = (struct sockaddr *) &addr;
    result = recvfrom(fd, raw, sizeof(raw), 0, sockaddr, &addrlen);
    if (result <= 0) {
        syswarn("cannot receive packet");
        return NULL;
    }

    /* Format the client address for logging. */
    if (!network_sockaddr_sprint(source, sizeof(source), sockaddr))
        syswarn("cannot convert client address to string");

    /* Ensure the packet is large enough to contain the header. */
    if ((size_t) result < sizeof(struct lbcd_header)) {
        warn("client %s: short packet received (length %lu)", source,
             (unsigned long) result);
        return NULL;
    }

    /* Extract the header fields. */
    packet = (struct lbcd_request *) raw;
    protocol  = ntohs(packet->h.version);
    id        = ntohs(packet->h.id);
    operation = ntohs(packet->h.op);
    nservices = ntohs(packet->h.status);

    /* Now, ensure the request packet is exactly the correct size. */
    expected = sizeof(struct lbcd_header);
    if (protocol == 3) {
        if (nservices > LBCD_MAX_SERVICES) {
            warn("client %s: too many services in request (%u)", source,
                 nservices);
            return NULL;
        }
        expected += nservices * sizeof(lbcd_name_type);
    }
    if ((size_t) result != expected) {
        warn("client %s: incorrect packet size (%lu != %lu)", source,
             (unsigned long) result, (unsigned long) expected);
        return NULL;
    }

    /* The packet appears valid.  Create the request struct. */
    request = xcalloc(1, sizeof(struct request));
    request->source    = xstrdup(source);
    request->addrlen   = addrlen;
    request->addr      = xmalloc(addrlen);
    memcpy(request->addr, &addr, addrlen);
    request->protocol  = protocol;
    request->id        = id;
    request->operation = operation;
    request->services  = vector_new();

    /* Check protocol number. */
    if (protocol != 2 && protocol != 3) {
        warn("client %s: protocol version %u unsupported", source, protocol);
        send_status(request, fd, LBCD_STATUS_VERSION);
        goto fail;
    }

    /*
     * Protocol version 3 takes a client-supplied list of services, with the
     * number of client-provided services given in the otherwise-unused status
     * field of the request header.
     */
    if (request->protocol == 3)
        for (i = 0; i < nservices; i++) {
            service = xstrndup(packet->names[i], sizeof(lbcd_name_type));
            if (!service_allowed(config, service)) {
                warn("client %s: service %s not allowed", source, service);
                send_status(request, fd, LBCD_STATUS_ERROR);
                goto fail;
            }
            vector_add(request->services, service);
            free(service);
        }
    return request;

fail:
    request_free(request);
    return NULL;
}


/*
 * Handle an incoming request.  Most of the work is done by lbcd_pack_info,
 * but this handles creating the header and sending the reply packet.
 */
static void
handle_lb_request(struct lbcd_config *config, struct request *request,
                  socket_type fd)
{
    struct lbcd_reply reply;
    size_t size, unused;
    ssize_t result;

    /* Log the request. */
    if (config->log)
        notice("request from %s (version %d)", request->source,
               request->protocol);

    /* Fill in reply header. */
    reply.h.version = htons(request->protocol);
    reply.h.id      = htons(request->id);
    reply.h.op      = htons(request->operation);
    reply.h.status  = htons(LBCD_STATUS_OK);

    /* Fill in reply. */
    lbcd_pack_info(&reply, request->protocol, request->services,
                   config->simple);

    /* Compute reply size (maximum packet minus unused service slots). */
    unused = LBCD_MAX_SERVICES - request->services->count;
    size = sizeof(reply) - unused * sizeof(struct lbcd_service);

    /* Send reply */
    result = sendto(fd, &reply, size, 0, request->addr, request->addrlen);
    if (result < 0 || (size_t) result != size)
        syswarn("client %s: cannot send reply", request->source);
}


/*
 * Given a bind address, return true if it's an IPv6 address.  Otherwise, it's
 * assumed to be an IPv4 address.
 */
#ifdef HAVE_INET6
static bool
is_ipv6(const char *string)
{
    struct in6_addr addr;
    return inet_pton(AF_INET6, string, &addr) == 1;
}
#else
static bool
is_ipv6(const char *string UNUSED)
{
    return false;
}
#endif


/*
 * Bind the listening socket or sockets on which we accept UDP requests and
 * return a list of sockets in the fds parameter.  Return a count of sockets
 * in the count parameter.
 *
 * Handle the socket activation case where the socket has already been set up
 * for us by systemd and, in that case, just return the already-configured
 * socket.
 */
static void
bind_socket(struct lbcd_config *config, socket_type **fds,
            unsigned int *count)
{
    int status;
    size_t i;
    const char *addr;

    /* Check whether systemd has already bound the socket. */
    status = sd_listen_fds(true);
    if (status < 0)
        die("using systemd-bound sockets failed: %s", strerror(-status));
    if (status > 0) {
        *fds = xcalloc(status, sizeof(socket_type));
        for (i = 0; i < (size_t) status; i++)
            (*fds)[i] = SD_LISTEN_FDS_START + i;
        return;
    }

    /*
     * We have to do the work ourselves.  If there is no bind address, bind
     * to all local sockets, which will normally result in two file
     * descriptors on which to listen.  If there is a bind address, bind only
     * to that address, whether IPv4 or IPv6.
     */
    if (config->bindaddrs->count == 0) {
        if (!network_bind_all(SOCK_DGRAM, config->port, fds, count))
            sysdie("cannot create UDP socket");
    } else {
        *count = config->bindaddrs->count;
        *fds = xcalloc(*count, sizeof(socket_type));
        for (i = 0; i < config->bindaddrs->count; i++) {
            addr = config->bindaddrs->strings[i];
            if (is_ipv6(addr))
                (*fds)[i] = network_bind_ipv6(SOCK_DGRAM, addr, config->port);
            else
                (*fds)[i] = network_bind_ipv4(SOCK_DGRAM, addr, config->port);
            if ((*fds)[i] == INVALID_SOCKET)
                sysdie("cannot bind to address: %s", addr);
        }
    }
}


/*
 * Set up our network connection and handle incoming requests.
 */
static void
handle_requests(struct lbcd_config *config)
{
    int status;
    socket_type *fds;
    unsigned int count;
    struct request *request;
    FILE *pid;

    /* Open UDP socket. */
    bind_socket(config, &fds, &count);

    /* Indicate to the world that we're ready to answer requests. */
    if (config->pid_file != NULL) {
        pid = fopen(config->pid_file, "w");
        if (pid == NULL)
            warn("cannot create PID file %s", config->pid_file);
        else {
            fprintf(pid, "%d\n", (int) getpid());
            fclose(pid);
        }
    }
    notice("ready to accept requests");

    /* Indicate to systemd that we're ready to answer requests. */
    status = sd_notify(true, "READY=1");
    if (status < 0)
        warn("cannot notify systemd of startup: %s", strerror(-status));

    /* Indicate to upstart that we're ready to answer requests. */
    if (config->upstart)
        if (raise(SIGSTOP) < 0)
            syswarn("cannot notify upstart of startup");

    /* Main loop.  Continue until we're signaled. */
    while (1) {
        fd_set readfds;
        socket_type maxfd, fd;
        unsigned int i;

        /* Check all of our bound sockets for an incoming message. */
        FD_ZERO(&readfds);
        maxfd = -1;
        for (i = 0; i < count; i++) {
            FD_SET(fds[i], &readfds);
            if (fds[i] > maxfd)
                maxfd = fds[i];
        }
        status = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (status < 0)
            sysdie("cannot select on bound sockets");

        /* Find the socket where we got a message. */
        fd = INVALID_SOCKET;
        for (i = 0; i < count; i++)
            if (FD_ISSET(fds[i], &readfds)) {
                fd = fds[i];
                break;
            }
        if (fd == INVALID_SOCKET)
            sysdie("select returned with no valid sockets");

        /* Accept and process the message. */
        request = request_recv(config, fd);
        if (request == NULL)
            continue;
        switch (request->operation) {
        case LBCD_OP_LBINFO:
            handle_lb_request(config, request, fd);
            break;
        default:
            warn("client %s: unknown op %d requested", request->source,
                 request->operation);
            send_status(request, fd, LBCD_STATUS_UNKNOWN_OP);
            break;
        }
        request_free(request);
    }
}


/*
 * Main routine.  Parse command-line options and then dispatch to the
 * appropriate functions.
 */
int
main(int argc, char **argv)
{
    struct lbcd_config config;
    int debugging = 0;
    int testmode = 0;
    int foreground = 0;
    char *lbcd_helper = NULL;
    const char *service_weight = NULL;
    int service_timeout = LBCD_TIMEOUT;
    int c;

    /* Establish identity. */
    message_program_name = "lbcd";

    /* A quick hack to honor --help and --version */
    if (argv[1] != NULL)
        if (argv[1][0] == '-' && argv[1][1] == '-' && argv[1][2] != '\0') {
            switch(argv[1][2]) {
            case 'h':
                usage(0);
            case 'v':
                version();
            default:
                usage(1);
            }
        }

    /* Initialize options. */
    memset(&config, 0, sizeof(config));
    config.bindaddrs = vector_new();
    config.port = LBCD_PORTNUM;
    config.services = vector_new();

    /* Parse the regular command-line options. */
    opterr = 1;
    while ((c = getopt(argc, argv, "a:b:c:dfhlP:p:RStT:w:Z")) != EOF) {
        switch (c) {
        case 'a': /* allowed service */
            vector_add(config.services, optarg);
            break;
        case 'b': /* bind address */
            vector_add(config.bindaddrs, optarg);
            break;
        case 'c': /* helper command -- must be full path to command */
            lbcd_helper = optarg;
            if (access(lbcd_helper, X_OK) != 0)
                sysdie("cannot access %s", optarg);
            break;
        case 'd': /* debugging mode */
            debugging = 1;
            foreground = 1;
            break;
        case 'f': /* run in foreground */
            foreground = 1;
            break;
        case 'h': /* usage */
            usage(0);
            break;
        case 'l': /* log requests */
            config.log = true;
            break;
        case 'P': /* pid file */
            config.pid_file = optarg;
            break;
        case 'p': /* port number */
            config.port = atoi(optarg);
            break;
        case 'R': /* round-robin */
            service_weight = "rr";
            break;
        case 'S': /* simple, no version two adjustments */
            config.simple = true;
            break;
        case 't': /* test mode */
            testmode = 1;
            break;
        case 'T': /* timeout */
            service_timeout = atoi(optarg);
            if (service_timeout < 1 || service_timeout > 300)
                die("timeout (%d) must be between 1 and 300 seconds",
                    service_timeout);
            break;
        case 'w': /* weight or service */
            service_weight = optarg;
            vector_add(config.services, optarg);
            break;
        case 'Z': /* raise(SIGSTOP) when ready for connections */
            config.upstart = true;
            break;
        default:
            usage(1);
            break;
        }
    }

    /* Initialize default load handler. */
    if (lbcd_weight_init(lbcd_helper, service_weight, service_timeout) != 0)
        die("cannot initialize service handler");

    /* If testing, print default output and terminate */
    if (testmode)
        lbcd_test(argc - optind, argv + optind);

    /*
     * Background ourself unless running in the foreground.  Do not chdir in
     * case we're running external probe programs that care about the current
     * working directory (although that's inadvisable).
     */
    if (!foreground)
        if (daemon(1, 0) < 0)
            sysdie("cannot daemonize");

    /* Switch to syslog logging unless debugging. */
    if (!debugging) {
        openlog("lbcd", LOG_PID | LOG_NDELAY, LOG_DAEMON);
        message_handlers_notice(1, message_log_syslog_info);
        message_handlers_warn(1, message_log_syslog_warning);
        message_handlers_die(1, message_log_syslog_err);
    }

    /* Become a daemon.  handle_requests never returns. */
    handle_requests(&config);
    return 0;
}
