/*
 * lbcd kernel code for BSD.
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
#include <kvm.h>
#include <nlist.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/times.h>

#include <lbcd.h>
#include <util/messages.h>

#define C_KMEM   "/dev/kmem"
#define C_VMUNIX "/bsd"

#ifndef PROGNAME
# define PROGNAME "lbcd"
#endif

static struct nlist nl[] = {
    { "_avenrun"  },
    { "_boottime" },
    { NULL        }
};
static kvm_t *kernel = NULL;
static int kernel_init = 0;


/*
 * Sets up for later queries.  Open the kernel memory file and find the
 * address of the necessary variable.  Exits on error.
 */
static int
kernel_open(void)
{
    kernel = kvm_open(NULL, NULL, NULL, O_RDONLY, PROGNAME);
    if (kernel == NULL)
        sysdie("kvm_open failed");
    if (kvm_nlist(kernel, nl) != 0)
        sysdie("kvm_nlist failed");
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
    return kvm_close(kd);
}


/*
 * Read a value from kernel memory, given its offset, and store it in the dest
 * buffer.  That buffer has space for dest_len octets.
 */
static void
kernel_read(off_t where, void *dest, int dest_len)
{
    int status;

    status = kvm_read(kernel, where, dest, dest_len);
    if (status != 0)
        sysdie("kvm_read failed");
}


/*
 * Get the current load average from the kernel and return the one minute,
 * five minute, and fifteen minute averages in the given parameters.  Returns
 * 0 on success and -1 on failure.
 */
int
kernel_getload(double *l1, double *l5, double *l15)
{
    long load[3];

    if (!kernel_init)
        kernel_open();
    if (nl[0].n_type == 0)
        return -1;
    kernel_read(nl[0].n_value, (void *) load, sizeof(load));
    *l1  = (double) load[0] / FSCALE;
    *l5  = (double) load[1] / FSCALE;
    *l15 = (double) load[2] / FSCALE;
    return 0;
}


/*
 * Get the system uptime and return it in the boottime parameter.  Returns 0
 * on success and -1 on failure.
 */
int
kernel_getboottime(time_t *boottime)
{
    struct timeval boot;

    if (!kernel_init)
        kernel_open();
    if (nl[1].n_type == 0)
        return -1;
    kernel_read(nl[0].n_value, (void *) &boot, sizeof(boot));
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

    if (kernel_getload(&l1, &l5, &l15) == 0)
        printf("load %.02f %.02f %.02f\n", l1, l5, l15);
    if (kernel_getboottime(&boottime) == 0)
        printf("booted at %s", ctime(&boottime));
    return 0;
}
#endif
