/*
 * lbcd kernel code for HP-UX 9.x.
 *
 * Written by Russ Allbery <eagle@eyrie.org>
 * Copyright 2000, 2009, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/system.h>

#include <fcntl.h>
#include <nlist.h>
#include <sys/param.h>
#include <sys/time.h>

#include <server/internal.h>
#include <util/messages.h>

#define C_KMEM   "/dev/kmem"
#define C_VMUNIX "/hp-ux"

static struct nlist nl[] = {
    { "avenrun"  },
    { "boottime" },
    { NULL       }
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
    if (kernel_fd < 0)
        sysdie("cannot open %s", C_KMEM);
    if (nlist(C_VMUNIX, nl) < 0)
        sysdie("no namelist for %s", C_VMUNIX);
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
static void
kernel_read(off_t where, void *dest, int dest_len)
{
    ssize_t status;

    if (lseek(kernel_fd, where, SEEK_SET) < 0)
        sysdie("cannot lseek in %s", C_KMEM);
    status = read(kernel_fd, dest, dest_len);
    if (status < 0)
        sysdie("kernel read of %s failed", C_KMEM);
}


/*
 * Get the current load average from the kernel and return the one minute,
 * five minute, and fifteen minute averages in the given parameters.  Returns
 * 0 on success and -1 on failure.
 */
int
kernel_getload(double *l1, double *l5, double *l15)
{
    double load[3];

    if (!kernel_init)
        kernel_open();
    if (nl[0].n_type == 0)
        return -1;
    kernel_read(nl[0].n_value, (void *) load, sizeof(load));
    *l1  = kern_avenrun[0];
    *l5  = kern_avenrun[1];
    *l15 = kern_avenrun[2];
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
    kernel_read(nl[1].n_value, (void *) &boot, sizeof(boot));
    *boottime = boot.tv_sec;
    return 0;
}
