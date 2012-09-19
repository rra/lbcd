/*
 * lbcd load module to check SMTP server.
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
#include <util/macros.h>


/*
 * Helper function delegating the work to probe_tcp.  Kept as a separate
 * function to make testing easier.
 */
static int
probe_smtp(const char *host, int timeout)
{
    return probe_tcp(host, "smtp", 25, "220", timeout);
}


/*
 * The module interface with the rest of lbcd.
 */
int
lbcd_smtp_weight(uint32_t *weight_val, uint32_t *incr_val UNUSED, int timeout,
                 const char *portarg UNUSED, P_LB_RESPONSE *lb UNUSED)
{
    return *weight_val = probe_smtp("localhost", timeout);
}


/*
 * Test routine.
 */
#ifdef MAIN
int
main(int argc, char *argv[])
{
    int status;

    status = probe_smtp(argv[1], 5);
    printf("sendmail service %savailable\n", status ? "not " : "");
    return status;
}
#endif
