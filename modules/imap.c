/*
 * lbcd load module to check IMAP server.
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

#include <lbcd/internal.h>
#include <modules/modules.h>
#include <util/macros.h>


/*
 * Given a host and timeout, probe the IMAP server.  The port is not
 * configurable.  The host defaults to localhost.  This closes down the IMAP
 * connection nicely rather than just closing the connection.
 */
static int
probe_imap(const char *host, int timeout)
{
    socket_type sd;
    int retval = 0;

    sd = tcp_connect(host ? host : "localhost", "imap", 143);
    if (sd == INVALID_SOCKET)
        return -1;
    else {
        retval = lbcd_check_reply(sd, timeout, "* OK");
        /* Only for clean shutdown, don't care about failure. */
        if (socket_write(sd, "tag logout\r\n", 12) < 0) {}
        socket_close(sd);
    }
    return retval;
}


/*
 * The module interface with the rest of lbcd.
 */
int
lbcd_imap_weight(uint32_t *weight_val, uint32_t *incr_val UNUSED, int timeout,
                 const char *portarg UNUSED, struct lbcd_reply *lb UNUSED)
{
    return *weight_val = probe_imap("localhost", timeout);
}


/*
 * Test routine.
 */
#ifdef MAIN
int
main(int argc, char *argv[])
{
    int status;

    status = probe_imap(argv[1]);
    printf("imap service %savailable\n", status ? "not " : "");
    return status;
}
#endif
