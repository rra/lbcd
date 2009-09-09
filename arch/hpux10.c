/*
 * lbcd kernel code for HP-UX 10.x.
 *
 * This will only work for HP-UX 10.x and later.  For earlier releases of
 * HP-UX, see arch/hpux9.c.
 *
 * Written by Larry Schwimmer
 * Copyright 1997, 2009 Board of Trustees, Leland Stanford Jr. University
 *
 * See LICENSE for licensing terms.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <fcntl.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/pstat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <utmp.h>
#include <utmpx.h>

#include "lbcd.h"


/*
 * Get the current load average from the kernel and return the one minute,
 * five minute, and fifteen minute averages in the given parameters.  Returns
 * 0 on success and -1 on failure.
 */
int
kernel_getload(double *l1, double *l5, double *l15)
{
    struct pst_dynamic dyn_info;

    if (pstat_getdynamic(&dyn_info, sizeof(dyn_info), 0, 0) < 0)
        return -1;
    *l1  = dyn_info.psd_avg_1_min;
    *l5  = dyn_info.psd_avg_5_min;
    *l15 = dyn_info.psd_avg_15_min;
    return 0;
}


/*
 * Get the system uptime and return it in the boottime parameter.  Returns 0
 * on success and -1 on failure.
 */
int
kernel_getboottime(time_t *boottime)
{
    int fd;

    *boottime = 0;
    fd = open(UTMPX_FILE, O_RDONLY | O_NONBLOCK);
    if (fd >= 0) {
        ssize_t nread;
        struct utmpx ut;

        while ((nread = read(fd, &ut, sizeof(ut))) > 0) {
            if (strcmp(BOOT_MSG, ut.ut_line) == 0) {
                *boottime = ut.ut_tv.tv_sec;
                break;
            }
        }
        close(fd);
    }
    return (*boottime == 0) ? -1 : 0;
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
