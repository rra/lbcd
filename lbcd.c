/*
 * The main lbcd server functionality.
 *
 * Handles parsing command-line options, setting up the daemon, waiting for
 * packets, and handling each incoming request.
 *
 * Written by Larry Schwimmer
 * Copyright 1996, 1997, 1998, 2005, 2006, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/socket.h>
#include <portable/system.h>

#include <errno.h>
#include <signal.h>
#include <syslog.h>

#include <lbcd.h>
#include <util/messages.h>

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
 * Given the file to which the PID was written, stop a running instance of
 * lbcd.  This is basically the same as kill -TERM `cat /path/to/file`.
 */
static int
stop_lbcd(const char *pid_file)
{
    pid_t pid;
    FILE *file;

    file = fopen(pid_file,"r");
    if (file == NULL)
        return 0;
    if (fscanf(file, "%d", (int *) &pid) < 1) {
        fclose(file);
        return 0;
    }
    fclose(file);
    if (pid != 0 && kill(pid, 0) == 0)
        if (kill(pid, SIGTERM) < 0) {
            syswarn("cannot kill PID %lu", (unsigned long) pid);
            return -1;
        }
    return 0;
}


/*
 * Send a status reply back to the client.
 */
static int
lbcd_send_status(int s, struct sockaddr_in *cli_addr, int cli_len,
                 P_HEADER *request_header, p_status_t pstat)
{
    P_HEADER header;
    char client[INET6_ADDRSTRLEN];

    header.version = htons(LBCD_VERSION);
    header.id      = htons(request_header->id);
    header.op      = htons(request_header->op);
    header.status  = htons(pstat);

    if (sendto(s, &header, sizeof(header), 0, (struct sockaddr *) cli_addr,
               cli_len) != sizeof(header)) {
        if (inet_ntop(AF_INET, cli_addr, client, sizeof(client)) == NULL)
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
lbcd_recv_udp(int s, struct sockaddr_in *cli_addr, socklen_t cli_len,
              char *mesg, int max_mesg)
{
    int n;
    P_HEADER_FULLPTR ph;
    char client[INET6_ADDRSTRLEN];

    n = recvfrom(s, mesg, max_mesg, 0, (struct sockaddr *) cli_addr, &cli_len);
    if (n < 0) {
        syswarn("cannot receive packet");
        return 0;
    }
    if (inet_ntop(AF_INET, cli_addr, client, sizeof(client)) == NULL) {
        syswarn("cannot convert client address to string");
        strlcpy(client, "UNKNOWN", sizeof(client));
    }
    if (n < (int) sizeof(P_HEADER)) {
        warn("client %s: short packet received (length %d)", client, n);
        return 0;
    }

    /* Convert request to host format. */
    ph = (P_HEADER_FULLPTR) mesg;
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
            ph->names[i][sizeof(LBCD_SERVICE_REQ) - 1] = '\0';
        break;
    }

    case 2:
        break;

    default:
        warn("client %s: protocol version %d unsupported", client,
             ph->h.version);
        lbcd_send_status(s, cli_addr, cli_len, &ph->h, status_lbcd_version);
        return 0;
    }
    return n;
}


/*
 * Handle an incoming request.
 */
static void
handle_lb_request(int s, P_HEADER_FULLPTR ph, struct sockaddr_in *cli_addr,
                  int cli_len, int simple)
{
    P_LB_RESPONSE lbr;
    int pkt_size;
    char client[INET6_ADDRSTRLEN];

    /* Fill in reply header. */
    lbr.h.version = htons(ph->h.version);
    lbr.h.id      = htons(ph->h.id);
    lbr.h.op      = htons(ph->h.op);
    lbr.h.status  = htons(status_ok);

    /* Fill in reply. */
    lbcd_pack_info(&lbr, ph, simple);

    /* Compute reply size (maximum packet minus unused service slots). */
    pkt_size = sizeof(lbr) -
        (LBCD_MAX_SERVICES - lbr.services) * sizeof(LBCD_SERVICE);

    /* Send reply */
    if (sendto(s, &lbr, pkt_size, 0, (const struct sockaddr *) cli_addr,
               cli_len) != pkt_size) {
        if (inet_ntop(AF_INET, cli_addr, client, sizeof(client)) == NULL)
            strlcpy(client, "UNKNOWN", sizeof(client));
        syswarn("client %s: cannot send reply", client);
    }
}


/*
 * Set up our network connection and handle incoming requests.
 */
static void
handle_requests(int port, const char *pid_file, struct in_addr *bind_address,
                int simple)
{
    int s;
    struct sockaddr_in serv_addr, cli_addr;
    int cli_len;
    int n;
    char mesg[LBCD_MAXMESG];
    char client[INET6_ADDRSTRLEN];
    P_HEADER_FULLPTR ph;
    FILE *pid;

    /* Open UDP socket. */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        sysdie("cannot create UDP socket");
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr = *bind_address;
    serv_addr.sin_port = htons(port);
    if (bind(s, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        sysdie("cannot bind UDP socket");

    /* Indicate to the world that we're ready to answer requests. */
    pid = fopen(pid_file, "w");
    if (pid == NULL)
        warn("cannot create PID file %s", pid_file);
    else {
        fprintf(pid, "%d\n", (int) getpid());
        fclose(pid);
    }
    notice("ready to accept requests");

    /* Main loop.  Continue until we're signaled. */
    while (1) {
        cli_len = sizeof(cli_addr);
        n = lbcd_recv_udp(s, &cli_addr, cli_len, mesg, sizeof(mesg));
        if (n > 0) {
            ph = (P_HEADER_FULLPTR) mesg;
            if (inet_ntop(AF_INET, &cli_addr, client, sizeof(client)) == NULL)
                strlcpy(client, "UNKNOWN", sizeof(client));
            switch (ph->h.op) {
            case op_lb_info_req:
                handle_lb_request(s, ph, &cli_addr, cli_len, simple);
                break;
            default:
                warn("client %s: unknown op %d requested", client, ph->h.op);
                lbcd_send_status(s, &cli_addr, cli_len, &ph->h,
                                 status_unknown_op);
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
    int debugging = 0;
    int port = LBCD_PORTNUM;
    int testmode = 0;
    int restart = 0;
    int simple = 0;
    const char *pid_file = PID_FILE;
    char *lbcd_helper = NULL;
    const char *service_weight = NULL;
    struct in_addr bind_address;
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

    /* Parse the regular command-line options. */
    opterr = 1;
    bind_address.s_addr = htonl(INADDR_ANY);
    while ((c = getopt(argc, argv, "P:Rb:c:dhlp:rSstT:w:")) != EOF) {
        switch (c) {
        case 'h': /* usage */
            usage(0);
            break;
        case 'P': /* pid file */
            pid_file = optarg;
            break;
        case 'R': /* round-robin */
            service_weight = "rr";
            break;
        case 'b': /* bind address */
            if (inet_aton(optarg, &bind_address) == 0)
                die("invalid bind address %s", optarg);
            break;
        case 'c': /* helper command -- must be full path to command */
            lbcd_helper = optarg;
            if (access(lbcd_helper, X_OK) != 0)
                sysdie("cannot access %s", optarg);
            break;
        case 'd': /* debugging mode */
            debugging = 1;
            break;
        case 'l': /* log requests */
            /* FIXME: implement */
            break;
        case 'p': /* port number */
            port = atoi(optarg);
            break;
        case 'r': /* restart */
            restart = 1;
            break;
        case 'S': /* simple, no version two adjustments */
            simple = 1;
            break;
        case 's': /* stop */
            exit(stop_lbcd(pid_file));
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
        default:
            usage(1);
            break;
        }
    }

    /* Handle restart, if requested. */
    if (!testmode && restart && stop_lbcd(pid_file) != 0)
        exit(1);

    /* Initialize default load handler. */
    if (lbcd_weight_init(lbcd_helper, service_weight, service_timeout) != 0)
        die("cannot initialize service handler");

    /* If testing, print default output and terminate */
    if (testmode)
        lbcd_test(argc - optind, argv + optind);

    /*
     * Background ourself and switch to syslog logging unless debugging.  Do
     * not chdir in case we're running external probe programs that care about
     * the current working directory (although that's inadvisable).
     */
    if (!debugging) {
        if (daemon(1, 0) < 0)
            sysdie("cannot daemonize");
        openlog("lbcd", LOG_PID | LOG_NDELAY, LOG_DAEMON);
        message_handlers_notice(1, message_log_syslog_info);
        message_handlers_warn(1, message_log_syslog_warning);
        message_handlers_die(1, message_log_syslog_err);
    }

    /* Become a daemon.  handle_requests never returns. */
    handle_requests(port, pid_file, &bind_address, simple);
    return 0;
}
