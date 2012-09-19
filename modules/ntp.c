/*
 * lbcd load module to check NTP server.
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
#include <modules/monlist.h>
#include <util/macros.h>


/*
 * Probe an NTP server and determine how many peers it has, returning that
 * number or -1 on an error.  Takes the host and a timeout and defaults to
 * probing localhost.
 */
static int
probe_ntp(const char *host, int timeout)
{
    int sd;
    int retval = 0;

    sd = udp_connect(host ? host : "localhost", "ntp", 123);
    if (sd == -1)
        return -1;
    else {
        retval = monlist(sd, timeout);
        close(sd);
    }
    return retval;
}


/*
 * The module interface with the rest of lbcd.
 */
int
lbcd_ntp_weight(u_int *weight_val, u_int *incr_val UNUSED, int timeout,
                const char *portarg UNUSED, P_LB_RESPONSE *lb UNUSED)
{
    *weight_val = (u_int) probe_ntp("localhost", timeout);
    return (*weight_val == (u_int) -1) ? -1 : 0;
}


/*
 * Test routine.
 */
#ifdef MAIN
int
main(int argc, char *argv[])
{
    int status;

    status = probe_ntp(argv[1]);
    if (status <= 0) {
        printf("ntp service not available\n");
        return -1;
    } else {
        printf("%s ntp service has %d peers\n",
               argv[1] ? argv[1] : "localhost",
               status);
    }
    return 0;
}
#endif
