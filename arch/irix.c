/* irix.c -- lbcd kernel code for IRIX.
   $Id$

   Written by Russ Allbery <rra@stanford.edu>
   Copyright 2000 Board of Trustees, Leland Stanford Jr. University

   Based on src/getloadavg.c from XEmacs 21.1.7.  Use utmpx to get the
   system boot time as the method used to get kernel addresses doesn't seem
   to work for the time of the last reboot. */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sysmp.h>
#include <sys/time.h>
#include <utmpx.h>

/* Whether we've opened the kernel already. */
static int kernel_init = 0;

/* The file descriptor of /dev/kmem, which we hold open. */
static int kmem_fd = -1;

int
kernel_close(void)
{
    if (!kernel_init) return 0;
    return close(kmem_fd);
}

int
kernel_getload(double *load1, double *load5, double *load15)
{
    int ldav_off, status, load[3];
    static long offset = -1;

    /* If necessary, open the system memory map. */
    if (!kernel_init) {
        kmem_fd = open("/dev/kmem", O_RDONLY);
        if (kmem_fd < 0) {
            util_log_error("open of /dev/kmem failed: %%m");
            exit(1);
        }
        kernel_init = 1;
    }

    /* Retrieve the kernel offset of the load average. */
    if (offset == -1) {
        ldav_off = sysmp(MP_KERNADDR, MPKA_AVENRUN);
        if (ldav_off == -1) {
            util_log_error("sysmp: %%m");
            exit(1);
        }
        offset = (long) ldav_off & 0x7fffffff;
    }
    
    /* Try to read the load. */
    if (lseek(kmem_fd, offset, 0) == -1L) {
        util_log_error("can't lseek in /dev/kmem: %%m");
        exit(1);
    }
    status = read(kmem_fd, (char *) load, sizeof(load));
    if (status != sizeof(load)) {
        util_log_error("read only %d bytes: %%m", status);
        exit(1);
    }

    *load1  = (double) load[0] / 1000.0;
    *load5  = (double) load[1] / 1000.0;
    *load15 = (double) load[2] / 1000.0;

    return 0;
}

int
kernel_getboottime(time_t *boottime)
{
    struct utmpx query;
    struct utmpx *result;

    query.ut_type = BOOT_TIME;
    setutxent();
    result = getutxid(&query);
    endutxent();
    if (!result) {
        util_log_error("getutxid failed to find BOOT_TIME");
        return -1;
    }
    *boottime = result->ut_tv.tv_sec;
    return 0;
}

#ifdef MAIN

#include <stdio.h>
#include <time.h>

int
main(void)
{
    double load1, load5, load15;
    time_t boottime;

    if (kernel_getload(&load1, &load5, &load15) == 0) {
        printf("load %.02f %.02f %.02f\n", load1, load5, load15);
    }
    if (kernel_getboottime(&boottime) == 0) {
        printf("booted at %s", ctime(&boottime));
    }

    return 0;
}

#endif /* MAIN */
