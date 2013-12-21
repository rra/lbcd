/*
 * lbcd kernel code for Mac OS X (Darwin).
 *
 * It uses the sysctl function to get the required information from the
 * kernel.  Works on Darwin 8.10.1 Mac OS X 10.4.10.  I make no guarantee or
 * warranty that it works with anything else.
 *
 * Written on 2007-11-15 by Clif Redding, Computer Science Dept.,
 *     University of West Florida
 * Updates by Russ Allbery <eagle@eyrie.org>
 * Copyright 2007 Clif Redding
 * Copyright 2009, 2012, 2013
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/system.h>

#include <fcntl.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/times.h>

#include <lbcd/internal.h>
#include <util/messages.h>


/*
 * Get the current load average from the kernel and return the one minute,
 * five minute, and fifteen minute averages in the given parameters.  Returns
 * 0 on success and -1 on failure.
 */
int
kernel_getload(double *l1, double *l5, double *l15)
{
    int mib[2];
    struct loadavg dyn_info;
    size_t size;

    mib[0] = CTL_VM;
    mib[1] = VM_LOADAVG;
    size = sizeof(dyn_info);
    if (sysctl(mib, 2, &dyn_info, &size, NULL, 0) < 0) {
        syswarn("error reading kernel load average");
        return -1;
    }

    *l1  = (double) dyn_info.ldavg[0] / FSCALE;
    *l5  = (double) dyn_info.ldavg[1] / FSCALE;
    *l15 = (double) dyn_info.ldavg[2] / FSCALE;

    return 0;
}


/*
 * Get the system uptime and return it in the boottime parameter.  Returns 0
 * on success and -1 on failure.
 */
int
kernel_getboottime(time_t *boottime)
{
    int mib[2];
    struct timeval boot;
    size_t size;

    mib[0] = CTL_KERN;
    mib[1] = KERN_BOOTTIME;
    size = sizeof(boot);
    if (sysctl(mib, 2, &boot, &size, NULL, 0) < 0) {
        syswarn("error reding kernel boot time");
        return -1;
    }
    *boottime = boot.tv_sec;
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

    if (kernel_getload(&l1,&l5,&l15) == 0)
        printf("load %.02f %.02f %.02f\n", l1, l5, l15);
    if (kernel_getboottime(&boottime) == 0)
        printf("booted at %s", ctime(&boottime));

    return 0;
}
#endif
