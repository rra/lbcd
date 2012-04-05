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
#include <portable/system.h>

#include <lbcdload.h>
#include <modules/modules.h>


/*
 * Given a host and timeout, probe the IMAP server.  The port is not
 * configurable.  The host defaults to localhost.  This closes down the IMAP
 * connection nicely rather than just closing the connection.
 */
static int
probe_imap(const char *host, int timeout)
{
    int sd;
    int retval = 0;

    sd = tcp_connect(host ? host : "localhost", "imap", 143);
    if (sd == -1)
        return -1;
    else {
        retval = lbcd_check_reply(sd, timeout, "* OK");
        write(sd, "tag logout\r\n", 12);
        close(sd);
    }
    return retval;
}


/*
 * The module interface with the rest of lbcd.
 */
int
lbcd_imap_weight(u_int *weight_val, u_int *incr_val UNUSED, int timeout,
                 const char *portarg UNUSED, P_LB_RESPONSE *lb UNUSED)
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
