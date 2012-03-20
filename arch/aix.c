/*
 * lbcd kernel code for AIX.
 *
 * Written by Larry Schiwmmer
 * Copyright 1997, 1998, 2009
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <fcntl.h>
#include <nlist.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>

#include "lbcd.h"

#define C_KMEM   "/dev/kmem"
#define C_VMUNIX "/unix"
#define FSCALE   65536.0

static struct nlist nl[] = {
    { "avenrun", 0, 0, 0, 0, 0 },
    { NULL,      0, 0, 0, 0, 0 },
};
static int kernel_fd   = -1;
static int kernel_init = 0;


/*
 * Sets up for later queries.  Open the kernel memory file and find the
 * address of the necessary variable.  Exits on error.
 */
static int
kernel_open(void)
{
    kernel_fd = open(C_KMEM, O_RDONLY);
    if (kernel_fd < 0) {
        util_log_error("can't open %s: %%m", C_KMEM);
        exit(1);
    }
    if (knlist(nl, 1, sizeof(struct nlist)) < 0) {
        util_log_error("no namelist for %s", C_VMUNIX);
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
    return close(kernel_fd);
}


/*
 * Read a value from kernel memory, given its offset, and store it in the dest
 * buffer.  That buffer has space for dest_len octets.
 */
static int
kernel_read(off_t where, void *dest, int dest_len)
{
    ssize_t status;

    if (lseek(kernel_fd, where, SEEK_SET) < 0) {
        util_log_error("can't lseek in %s: %%m", C_KMEM);
        exit(1);
    }
    status = read(kernel_fd, dest, dest_len);
    if (status < 0) {
        util_log_error("kernel read %s: %%m", C_KMEM);
        exit(1);
    }
    return status;
}


/*
 * Get the current load average from the kernel and return the one minute,
 * five minute, and fifteen minute averages in the given parameters.  Returns
 * 0 on success and -1 on failure.
 */
int
kernel_getload(double *l1, double *l5, double *l15)
{
    int load[3], status;

    if (!kernel_init)
        kernel_open();
    status = kernel_read(nl[0].n_value, (void *) load, sizeof(load));
    if (status < 0) {
        util_log_error("kernel loadaverage read error: %%m");
        return -1;
    }
    *l1  = load[0] / FSCALE;
    *l5  = load[1] / FSCALE;
    *l15 = load[2] / FSCALE;
    return 0;
}


/*
 * Get the system uptime and return it in the boottime parameter.  Returns 0
 * on success and -1 on failure.
 */
int
kernel_getboottime(time_t *boottime)
{
    time_t uptime, now;
    struct tms tbuf;

    uptime = times(&tbuf) / HZ;
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
