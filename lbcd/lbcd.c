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

/* The usage message. */
const char usage_message[] = "\
Usage: lbcd [options] [-d] [-p <port>]\n\
   -c <cmd>     run <cmd> (full path) to obtain load values\n\
   -d           debug mode, don't fork off\n\
   -h, --help   print usage\n\
   -l           log various requests\n\
   -P <file>    write PID to <file>\n\
   -p <port>    run using different port number\n\
   -r           restart (kill current lbcd)\n\
   -R           round-robin polling\n\
   -s           stop running lbcd\n\
   -w <option>  specify returned weight; options:\n\
                  either \"load:incr\" or \"service\"\n\
   -t           test mode (print stats and exit)\n\
   -T <seconds> timeout (1-300 seconds, default 5)\n\
   --version    print protocol version and exit\n";

/* Stores configuration information for lbcd. */
struct lbcd_config {
    struct in_addr bind_address; /* Address to listen on. */
    unsigned short port;         /* Port to listen on. */
    const char *pid_file;        /* Write the daemon PID to this path. */
    bool simple;                 /* Do not adjust results for version 2. */
    bool log;                    /* Log each request. */
    bool upstart;                /* Raise SIGSTOP when ready for upstart. */
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
 * Send a status reply back to the client.
 */
static int
lbcd_send_status(int s, struct sockaddr *cli_addr, socklen_t cli_len,
                 struct lbcd_header *request_header, enum lbcd_status pstat)
{
    struct lbcd_header header;
    char client[INET6_ADDRSTRLEN];
    ssize_t result;

    /* Build the response packet. */
    header.version = htons(LBCD_VERSION);
    header.id      = htons(request_header->id);
    header.op      = htons(request_header->op);
    header.status  = htons(pstat);

    /* Send the packet to the client. */
    result = sendto(s, &header, sizeof(header), 0, cli_addr, cli_len);
    if (result != sizeof(header)) {
        if (!network_sockaddr_sprint(client, sizeof(client), cli_addr))
            strlcpy(client, "UNKNOWN", sizeof(client));
        syswarn("client %s: cannot send reply", client);
        return -1;
    }
    return 0;
}


/*
 * Receive request packet and verify the integrity and format of the reply.
 * This routine is REQUIRED to sanitize the request packet.  All other program
 * routines can expect that the packet is safe to read once it is passed on.
 *
 * Returns the number of bytes read.
 */
static int
lbcd_recv_udp(int s, struct sockaddr *cli_addr, socklen_t *cli_len,
              void *mesg, int max_mesg)
{
    ssize_t n;
    struct lbcd_request *ph;
    char client[INET6_ADDRSTRLEN];

    n = recvfrom(s, mesg, max_mesg, 0, cli_addr, cli_len);
    if (n < 0) {
        syswarn("cannot receive packet");
        return 0;
    }
    if (!network_sockaddr_sprint(client, sizeof(client), cli_addr)) {
        syswarn("cannot convert client address to string");
        strlcpy(client, "UNKNOWN", sizeof(client));
    }
    if ((size_t) n < sizeof(struct lbcd_header)) {
        warn("client %s: short packet received (length %lu)", client,
             (unsigned long) n);
        return 0;
    }

    /* Convert request to host format. */
    ph = mesg;
    ph->h.version = ntohs(ph->h.version);
    ph->h.id      = ntohs(ph->h.id);
    ph->h.op      = ntohs(ph->h.op);
    ph->h.status  = ntohs(ph->h.status);

    /*
     * Check protocol number and packet integrity.
     *
     * Protocol version 3 takes a client-supplied load protocol.  Protocol
     * version 2 is the original protocol.  Everything else isn't supported.
     */
    switch(ph->h.version) {
    case 3: {
        int i;

        /*
         * Extended services request: status > 0.  Trim the query to the
         * maximum allowed services and ensure nul-termination of service
         * name.
         */
        if (ph->h.status > LBCD_MAX_SERVICES)
            ph->h.status = LBCD_MAX_SERVICES;
        for (i = 0; i < ph->h.status; i++)
            ph->names[i][sizeof(ph->names[i]) - 1] = '\0';
        break;
    }

    case 2:
        break;

    default:
        warn("client %s: protocol version %d unsupported", client,
             ph->h.version);
        lbcd_send_status(s, cli_addr, *cli_len, &ph->h, LBCD_STATUS_VERSION);
        return 0;
    }
    return n;
}


/*
 * Handle an incoming request.
 */
static void
handle_lb_request(struct lbcd_config *config, int s, struct lbcd_request *ph,
                  struct sockaddr *cli_addr, socklen_t cli_len)
{
    struct lbcd_reply lbr;
    int pkt_size;
    char client[INET6_ADDRSTRLEN];
    ssize_t result;

    /* Log the request. */
    if (config->log) {
        if (!network_sockaddr_sprint(client, sizeof(client), cli_addr))
            strlcpy(client, "UNKNOWN", sizeof(client));
        notice("request from %s (version %d)", client, ph->h.version);
    }

    /* Fill in reply header. */
    lbr.h.version = htons(ph->h.version);
    lbr.h.id      = htons(ph->h.id);
    lbr.h.op      = htons(ph->h.op);
    lbr.h.status  = htons(LBCD_STATUS_OK);

    /* Fill in reply. */
    lbcd_pack_info(&lbr, ph, config->simple);

    /* Compute reply size (maximum packet minus unused service slots). */
    pkt_size = sizeof(lbr) -
        (LBCD_MAX_SERVICES - lbr.services) * sizeof(struct lbcd_service);

    /* Send reply */
    result = sendto(s, &lbr, pkt_size, 0, cli_addr, cli_len);
    if (result != pkt_size) {
        if (!network_sockaddr_sprint(client, sizeof(client), cli_addr))
            strlcpy(client, "UNKNOWN", sizeof(client));
        syswarn("client %s: cannot send reply", client);
    }
}


/*
 * Bind the listening socket on which we accept UDP requests.  Handle the
 * socket activation case where the socket has already been set up for us by
 * systemd and, in that case, just return the already-configured socket.
 */
static int
bind_socket(int port, struct in_addr *bind_address)
{
    int s, socket_count;
    struct sockaddr_in addr;

    /* Check whether systemd has already bound the socket. */
    socket_count = sd_listen_fds(true);
    if (socket_count < 0)
        die("using systemd-bound sockets failed: %s", strerror(-socket_count));
    if (socket_count > 0)
        return SD_LISTEN_FDS_START;

    /* We have to do the work ourselves. */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        sysdie("cannot create UDP socket");
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = *bind_address;
    addr.sin_port = htons(port);
    if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        sysdie("cannot bind UDP socket");
    return s;
}


/*
 * Set up our network connection and handle incoming requests.
 */
static void
handle_requests(struct lbcd_config *config)
{
    int s, status, n;
    struct sockaddr_storage cli_addr;
    socklen_t cli_len;
    char mesg[LBCD_MAXMESG];
    char client[INET6_ADDRSTRLEN];
    struct lbcd_request *ph;
    FILE *pid;

    /* Open UDP socket. */
    s = bind_socket(config->port, &config->bind_address);

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
        cli_len = sizeof(cli_addr);
        n = lbcd_recv_udp(s, (struct sockaddr *) &cli_addr, &cli_len, mesg,
                          sizeof(mesg));
        if (n > 0) {
            ph = (struct lbcd_request *) mesg;
            if (!network_sockaddr_sprint(client, sizeof(client),
                                         (struct sockaddr *) &cli_addr))
                strlcpy(client, "UNKNOWN", sizeof(client));
            switch (ph->h.op) {
            case LBCD_OP_LBINFO:
                handle_lb_request(config, s, ph,
                                  (struct sockaddr *) &cli_addr, cli_len);
                break;
            default:
                warn("client %s: unknown op %d requested", client, ph->h.op);
                lbcd_send_status(s, (struct sockaddr *) &cli_addr, cli_len,
                                 &ph->h, LBCD_STATUS_UNKNOWN_OP);
            }
        }
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

    /* Set configuration defaults. */
    memset(&config, 0, sizeof(config));
    config.bind_address.s_addr = htonl(INADDR_ANY);
    config.port = LBCD_PORTNUM;

    /* Parse the regular command-line options. */
    opterr = 1;
    while ((c = getopt(argc, argv, "b:c:dfhlP:p:RStT:w:Z")) != EOF) {
        switch (c) {
        case 'b': /* bind address */
            if (inet_aton(optarg, &config.bind_address) == 0)
                die("invalid bind address %s", optarg);
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
