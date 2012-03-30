/*
 * lbcd load module to check NNTP server.
 *
 * Written by Larry Schwimmer
 * Copyright 1997, 1998, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "lbcdload.h"
#include "modules/modules.h"


/*
 * Helper function delegating the work to probe_tcp.  Kept as a separate
 * function to make testing easier.
 */
static int
probe_nntp(const char *host, int timeout)
{
    return probe_tcp(host, "nntp", 119, "200", timeout);
}


/*
 * The module interface with the rest of lbcd.
 */
int
lbcd_nntp_weight(u_int *weight_val, u_int *incr_val UNUSED, int timeout,
                 const char *portarg UNUSED, P_LB_RESPONSE *lb UNUSED)
{
    return *weight_val = probe_nntp("localhost", timeout);
}


/*
 * Test routine.
 */
#ifdef MAIN
int
main(int argc, char *argv[])
{
    int status;

    status = probe_nntp(argv[1], 5);
    printf("nntp service %savailable\n", status ? "not " : "");
    return status;
}
#endif
