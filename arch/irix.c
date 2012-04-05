/*
 * lbcd kernel code for IRIX.
 *
 * Use utmpx to get the system boot time as the method used to get kernel
 * addresses doesn't seem to work for the time of the last reboot.
 *
 * Written by Russ Allbery <rra@stanford.edu>
 * Copyright 2000, 2009, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/system.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmp.h>
#include <sys/time.h>
#include <time.h>
#include <utmpx.h>

#include <lbcd.h>

/* Whether we've opened the kernel already. */
static int kernel_init = 0;

/* The file descriptor of /dev/kmem, which we hold open. */
static int kmem_fd = -1;

/* The offset in /dev/kmem where the load average is stored. */
static long kmem_offset = -1;


/*
 * Sets up for later queries.  Open the kernel memory file and find the
 * address of the necessary variable.  Exits on error.
 */
static int
kernel_open(void)
{
    int ldav_off;

    kmem_fd = open("/dev/kmem", O_RDONLY);
    if (kmem_fd < 0) {
        util_log_error("open of /dev/kmem failed: %%m");
        exit(1);
    }
    ldav_off = sysmp(MP_KERNADDR, MPKA_AVENRUN);
    if (ldav_off < 0) {
        util_log_error("sysmp: %%m");
        exit(1);
    }
    kernel_offset = (long) ldav_off & 0x7fffffff;
    kernel_init = 1;
}


/*
 * Close and free any resources for querying the kernel.
 */
static int
kernel_close(void)
{
    if (!kernel_init)
        return 0;
    return close(kmem_fd);
}


/*
 * Get the current load average from the kernel and return the one minute,
 * five minute, and fifteen minute averages in the given parameters.  Returns
 * 0 on success and -1 on failure.
 */
int
kernel_getload(double *load1, double *load5, double *load15)
{
    int status, load[3];
    static long offset = -1;

    if (!kernel_init)
        kernel_open();
    if (lseek(kmem_fd, offset, 0) < 0) {
        util_log_error("can't lseek in /dev/kmem: %%m");
        exit(1);
    }
    status = read(kmem_fd, (void *) load, sizeof(load));
    if (status != sizeof(load)) {
        util_log_error("read only %d bytes: %%m", status);
        exit(1);
    }
    *load1  = (double) load[0] / 1000.0;
    *load5  = (double) load[1] / 1000.0;
    *load15 = (double) load[2] / 1000.0;
    return 0;
}


/*
 * Get the system uptime and return it in the boottime parameter.  Returns 0
 * on success and -1 on failure.
 */
int
kernel_getboottime(time_t *boottime)
{
    struct utmpx query, *result;

    query.ut_type = BOOT_TIME;
    setutxent();
    result = getutxid(&query);
    endutxent();
    if (result == NULL) {
        util_log_error("getutxid failed to find BOOT_TIME");
        return -1;
    }
    *boottime = result->ut_tv.tv_sec;
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
