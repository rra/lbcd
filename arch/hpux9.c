/*
 * lbcd kernel code for HP-UX
 */

#include <stdio.h>
#include <fcntl.h>
#include <nlist.h>
#include <sys/param.h>
#include <sys/time.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "lbcd.h"

#define C_KMEM "/dev/kmem"
#define C_VMUNIX "/hp-ux"

static int kernel_init = 0;

static int kd = -1;

static struct nlist nl[] = {
  { "avenrun" },         /* load average */
#define K_NL_LOADAVERAGE 0
  { "boottime" },         /* boottime */
#define K_NL_BOOTTIME 1
  { 0 }
};

static int
kernel_open(void)
{
  if ((kd = open(C_KMEM, O_RDONLY)) < 0) {
    util_log_error("open: %%m");
    exit(1);
 }

 if (nlist(C_VMUNIX, nl) !=0) {
    util_log_error("nlist error");
    exit(1);
 }

  kernel_init = 1;
  return 0;
}

static int
kernel_close(void)
{
  if (! kernel_init) return 0;
  return close(kd);
}

static int
kernel_read(off_t where, void *dest, int dest_len)
{
   int stat;

   if (lseek(kd,where,SEEK_SET)==-1) {
        util_log_error("can't lseek in %s: %%m",C_KMEM);
        exit(1);
   }

   stat = read(kd, dest, dest_len);
   if (stat==-1) {
     util_log_error("read : %%m");
     exit(1);
   }   
   return stat;
}

int
kernel_getload(double *l1, double *l5, double *l15)
{
  double kern_avenrun[3];

  if (!kernel_init) kernel_open();

  if (nl[K_NL_LOADAVERAGE].n_type==0) return -1;

  if (kernel_read( nl[K_NL_LOADAVERAGE].n_value, 
                       (char *) kern_avenrun,sizeof(kern_avenrun))==-1) { 
     util_log_error("kernel loadaverage read error: %%m");
      return -1;
  }

  *l1 = kern_avenrun[0];
  *l5 = kern_avenrun[1];
  *l15 =kern_avenrun[2];

  return 0;
}

int
kernel_getboottime(time_t *boottime)
{
  struct timeval boot;

  if (!kernel_init) kernel_open();

  if (nl[K_NL_BOOTTIME].n_type==0) return -1;

  if (kernel_read( (off_t)nl[K_NL_BOOTTIME].n_value, 
                       (char *) &boot,sizeof(boot))==-1) {
     util_log_error("kernel boottime read error: %%m"); 
      return -1;
  }

  *boottime = boot.tv_sec;

  return 0;
}


