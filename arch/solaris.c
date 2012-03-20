/*
 * lbcd kernel code for Solaris
 *
 * Uses kstat code in the Solaris FAQ written by Casper Dik in the
 * comp.unix.solaris post <3qh88s$6ho@engnews2.Eng.Sun.COM>.
 *
 * Written by Larry Schwimmer
 * Copyright 1997, 1998, 2009
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <fcntl.h>
#include <kstat.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>
#include <utmp.h>
#include <utmpx.h>

#include "lbcd.h"

static int kernel_init = 0;
static kstat_ctl_t *kernel = NULL;


/*
 * Sets up for later queries.  Open the kernel.  Exits on error.
 */
static int
kernel_open(void)
{
    kernel = kstat_open();
    if (kernel == NULL) {
        util_log_error("kstat_open failed: %%m");
        exit(1);
    }
    kernel_init = 1;
    return 0;
}


/*
 * Close and free any resources for querying the kernel.
 */
static int
kernel_close(void)
{
    if (!kernel_init)
        return 0;
    return kstat_close(kernel);
}


/*
 * Get the current load average from the kernel and return the one minute,
 * five minute, and fifteen minute averages in the given parameters.  Returns
 * 0 on success and -1 on failure.
 */
int
kernel_getload(double *l1, double *l5, double *l15)
{
    kstat_t *ksp;
    kstat_named_t *kn1, *kn5, *kn15;

    if (!kernel_init)
        kernel_open();
    ksp = kstat_lookup(kernel, "unix", 0, "system_misc");
    if (ksp == NULL)
        return -1;
    if (kstat_read(kernel, ksp, 0) < 0)
        return -1;
    kn1  = kstat_data_lookup(ksp, "avenrun_1min");
    kn5  = kstat_data_lookup(ksp, "avenrun_5min");
    kn15 = kstat_data_lookup(ksp, "avenrun_15min");
    if (kn1 == NULL || kn5 == NULL || kn15 == NULL)
        return -1;
    *l1  = (double) kn1->value.ul  / FSCALE;
    *l5  = (double) kn5->value.ul  / FSCALE;
    *l15 = (double) kn15->value.ul / FSCALE;
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
