/*
 * lbcd kernel code for Linux.
 *
 * Written by Larry Schwimmer
 * Copyright 1997, 1998, 2009, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/system.h>

#include <time.h>

#include <server/internal.h>
#include <util/messages.h>


/*
 * Get the current load average from the kernel and return the one minute,
 * five minute, and fifteen minute averages in the given parameters.  Returns
 * 0 on success and -1 on failure.
 */
int
kernel_getload(double *l1, double *l5, double *l15)
{
    FILE *fp;

    fp = fopen("/proc/loadavg", "r");
    if (fp == NULL) {
        syswarn("cannot open /proc/loadavg");
        return -1;
    }
    if (fscanf(fp, "%lf %lf %lf", l1, l5, l15) < 3) {
        fclose(fp);
        warn("cannot parse /proc/loadavg");
        return -1;
    }
    fclose(fp);
    return 0;
}


/*
 * Get the system uptime and return it in the boottime parameter.  Returns 0
 * on success and -1 on failure.
 */
int
kernel_getboottime(time_t *boottime)
{
    FILE *fp;
    double uptime;
    time_t curr;

    fp = fopen("/proc/uptime", "r");
    if (fp == NULL) {
        syswarn("cannot open /proc/uptime");
        return -1;
    }
    if (fscanf(fp, "%lf", &uptime) < 1) {
        fclose(fp);
        warn("cannot parse /proc/uptime");
        return -1;
    }
    fclose(fp);
    curr = time(NULL);
    *boottime = curr - uptime;
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
