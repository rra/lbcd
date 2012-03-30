/*
 * lbcd load module to check FTP server.
 *
 * Written by Larry Schwimmer
 * Copyright 1999, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "lbcd.h"
#include "lbcdload.h"
#include "modules/modules.h"


/*
 * Helper function delegating the work to probe_tcp.  Kept as a separate
 * function to make testing easier.
 */
static int
probe_ftp(const char *host, int timeout)
{
    return probe_tcp(host, "ftp", 21, "220", timeout);
}


/*
 * The module interface with the rest of lbcd.
 */
int
lbcd_ftp_weight(u_int *weight_val, u_int *incr_val UNUSED, int timeout,
                const char *portarg UNUSED, P_LB_RESPONSE *lb UNUSED)
{
    *weight_val = probe_ftp("localhost", timeout);
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

    status = probe_ftp(argv[1], 5);
    printf("ftp service %savailable\n", status ? "not " : "");
    return status;
}
#endif
