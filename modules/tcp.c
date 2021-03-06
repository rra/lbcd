/*
 * lbcd load module to check an arbitrary TCP service.
 *
 * Written by Larry Schwimmer
 * Copyright 1998, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/socket.h>
#include <portable/system.h>

#include <ctype.h>

#include <server/internal.h>
#include <modules/modules.h>
#include <util/macros.h>


/*
 * Generic TCP service probe.  Takes the host, the service name, the port,
 * an optional string to look for in the reply, and a timeout.  Returns 0 on
 * success and -1 on failure.
 */
int
probe_tcp(const char *host, const char *service, short port,
          const char *replycheck, int timeout)
{
    socket_type sd;
    int retval = 0;

    sd = tcp_connect(host ? host : "localhost", service, port);
    if (sd == INVALID_SOCKET)
        return -1;
    else {
        if (replycheck) {
            retval = lbcd_check_reply(sd, timeout, replycheck);
            /* Only for clean shutdown, don't care about failure. */
            if (socket_write(sd, "quit\r\n", 6) < 0) {}
        }
        socket_close(sd);
    }
    return retval;
}


/*
 * The module interface with the rest of lbcd.  Unlike many of the module
 * interfaces, this requires the port argument to determine which TCP port to
 * probe.  Always probes localhost.  Returns the weight or -1 on error.
 */
int
lbcd_tcp_weight(uint32_t *weight_val, uint32_t *incr_val UNUSED,
                int timeout, const char *portarg, struct lbcd_reply *lb UNUSED)
{
    const char *service = NULL;
    short port = 0;
    const char *cp;

    /*
     * Parse portarg.  If it's all digits, treat it as a port number;
     * otherwise, treat it as a service.
     */
    if (portarg == NULL)
        return -1;
    for (cp = portarg; *cp != '\0'; cp++) {
        if (!isdigit((int) *cp)) {
            service = portarg;
            break;
        }
    }
    if (*cp == '\0') {
        port = (short) atoi(portarg);
        if (port < 1)
            return -1;
    }

    /* Connect to the port, set weight, and return */
    *weight_val = (uint32_t) probe_tcp("localhost", service, port, NULL,
                                       timeout);
    return *weight_val;
}


/*
 * Test routine.
 */
#ifdef MAIN
int
main(int argc, char *argv[])
{
    int status;
    uint32_t i;

    status = probe_tcp(argv[1], "smtp", 25, "220", 5);
    printf("%s service %savailable\n", "smtp", status ? "not " : "");
    if (lbcd_tcp_weight(&i, &i, 5, "smtp") == 0)
        printf("service available\n");
    else
        printf("service not available\n");
    return status;
}
#endif
