/*
 * lbcd kernel code for BSD
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/times.h>
#include <nlist.h>
#include <kvm.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <utmp.h>
#include <utmpx.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lbcd.h"

#define C_KMEM "/dev/kmem"
#define C_VMUNIX "/bsd"

#ifndef PROGNAME
#define PROGNAME "lbcd"
#endif

static struct nlist nl[] = {
  { "_avenrun" },         /* load average */
#define K_NL_LOADAVERAGE 0
  { "_boottime" },         /* boottime */
#define K_NL_BOOTTIME 1
  { 0 }
};

static kvm_t *kd;

static int kernel_init = 0;

static int
kernel_open(void)
{
 if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, PROGNAME)) == NULL) {
    util_log_error("kvm_open: %%m");
    exit(1);
 }

 if (kvm_nlist(kd, nl) !=0) {
    util_log_error("kvm_nlist error");
    exit(1);
 }

  kernel_init = 1;
  return 0;
}

static int
kernel_close(void)
{
  if (! kernel_init) return;
  return kvm_close(kd);
}

static int
kernel_read(off_t where, void *dest, int dest_len)
{
   int stat;

   if ((stat = kvm_read(kd, where, dest, dest_len)) != 0) {
     util_log_error("kvm_read : %%m");
     exit(1);
   }   
   return stat;
}

int
kernel_getload(double *l1, double *l5, double *l15)
{
  long kern_avenrun[3];

  if (!kernel_init) kernel_open();

  if (nl[K_NL_LOADAVERAGE].n_type==0) return -1;

  if (kernel_read( nl[K_NL_LOADAVERAGE].n_value, 
		   (char *) kern_avenrun,sizeof(kern_avenrun))==-1) { 
    util_log_error("kernel loadaverage read error: %%m");
    return -1;
  }

  *l1  = (double)kern_avenrun[0]/FSCALE;
  *l5  = (double)kern_avenrun[1]/FSCALE;
  *l15 = (double)kern_avenrun[2]/FSCALE;

  return 0;
}

int
kernel_getboottime(time_t *boottime)
{
  struct timeval boot;

  if (!kernel_init) kernel_open();

  if (nl[K_NL_BOOTTIME].n_type==0) return -1;

  if (kernel_read( nl[K_NL_BOOTTIME].n_value, 
                       (char *) &boot,sizeof(boot))==-1) {
     util_log_error("kernel boottime read error: %%m"); 
      return -1;
  }

  *boottime = boot.tv_sec;

  return 0;
}

#ifdef MAIN
int
main()
{
  double l1,l5,l15;
  time_t boottime;

  if (kernel_getload(&l1,&l5,&l15) == 0) {
    printf("load %.02f %.02f %.02f\n",l1,l5,l15);
  }
  if (kernel_getboottime(&boottime) == 0) {
    printf("booted at %s",ctime(&boottime));
  }

  return 0;
}
#endif
