/*
 * lbcd load module to check HTTP server.
 *
 * Written by Larry Schwimmer
 * Copyright 1997, 1998, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/socket.h>
#include <portable/system.h>

#include <ctype.h>
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#include <sys/time.h>

#include <server/internal.h>
#include <modules/modules.h>
#include <util/macros.h>

/* The most basic HTTP query that we can perform. */
#define QUERY "GET / HTTP/1.0\r\n\r\n"


/*
 * Probe an HTTP server.  Takes the hostname, the timeout, and an optional
 * port as a string which is taken to be a port number.  If host is NULL,
 * localhost is used.  If the port is not given, probes port 80.  Returns
 * success (0) if and only if the HTTP server returns a status code of 200, so
 * cannot be used with a server that returns a redirect for /.  Returns -1 on
 * failure.
 */
static int
probe_http(const char *host, int timeout, const char *portarg)
{
    socket_type sd;
    int retval = 0;
    short port = 80;
    const char *service = "http";

    if (portarg != NULL) {
        port = (short) atoi(portarg);
        if (port < 1)
            port = 80;
        else
            service = NULL;     /* Force use of supplied port number. */
    }
    sd = tcp_connect(host ? host : "localhost", service, port);
    if (sd == INVALID_SOCKET)
        return -1;
    else {
        struct timeval tv = { 0, 0 };
        fd_set rset;
        char buf[17];
        char *p;

        tv.tv_sec = timeout;
        if (socket_write(sd, QUERY, sizeof(QUERY)) < (ssize_t) sizeof(QUERY)) {
            socket_close(sd);
            return -1;
        }
        FD_ZERO(&rset);
        FD_SET(sd, &rset);
        retval = -1;
        if (select(sd + 1, &rset, NULL, NULL, &tv) > 0) {
            buf[sizeof(buf) - 1] = '\0';

            /* Look for a response starting with 20x or 30x. */
            if (socket_read(sd, buf, sizeof(buf) - 1) > 0) {
                p = strstr(buf, "20");
                if (p == NULL)
                    p = strstr(buf, "30");
                if (p != NULL && isdigit((int) p[2]))
                    retval = 0;
            }
        }
        socket_close(sd);
    }
    return retval;
}


/*
 * The module interface with the rest of lbcd.  Only supports probing
 * localhost.
 */
int
lbcd_http_weight(uint32_t *weight_val, uint32_t *incr_val UNUSED,
		 int timeout, const char *portarg,
                 struct lbcd_reply *lb UNUSED)
{
    return *weight_val = probe_http("localhost", timeout, portarg);
}


/*
 * Test routine.
 */
#ifdef MAIN
int
main(int argc, char *argv[])
{
    int status;

    status = probe_http(argv[1], 5, NULL);
    printf("http service %savailable\n", status ? "not " : "");
    return status;
}
#endif
