/*
 * lbcd kernel code for Tru64 (Digital UNIX, OSF/1).
 *
 * Written by Larry Schwimmer
 * Copyright 1997, 1998, 2009, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/system.h>

#include <fcntl.h>
#include <sys/table.h>

#include <lbcd/internal.h>


/*
 * Get the current load average from the kernel and return the one minute,
 * five minute, and fifteen minute averages in the given parameters.  Returns
 * 0 on success and -1 on failure.
 */
int
kernel_getload(double *l1, double *l5, double *l15)
{
    struct tbl_loadavg load;

    if (table(TBL_LOADAVG, 0, &load, 1, sizeof(load)) < 0)
        return -1;

    if (load.tl_lscale != 0) {
        *l1  = load.tl_avenrun.l[0] / (float) load.tl_lscale;
        *l5  = load.tl_avenrun.l[1] / (float) load.tl_lscale;
        *l15 = load.tl_avenrun.l[2] / (float) load.tl_lscale;
    } else {
        *l1  = load.tl_avenrun.d[0];
        *l5  = load.tl_avenrun.d[1];
        *l15 = load.tl_avenrun.d[2];
    }
    return 0;
}


/*
 * Get the system uptime and return it in the boottime parameter.  Returns 0
 * on success and -1 on failure.
 */
int
kernel_getboottime(time_t *boottime)
{
    struct tbl_sysinfo sys;

    if (table(TBL_SYSINFO, 0, &sys, 1, sizeof(sys)) < 0)
        return -1;
    *boottime = sys.si_boottime;
    return 0;
}


/*
 * Test routine.
 */
#ifdef MAIN
int
main(void)
{
    double l1, l5, l15;
    time_t boottime;

    if (kernel_getload(&l1, &l5, &l15) == 0)
        printf("load %.02f %.02f %.02f\n", l1, l5, l15);
    if (kernel_getboottime(&boottime) == 0)
        printf("booted at %s", ctime(&boottime));
    return 0;
}
#endif
