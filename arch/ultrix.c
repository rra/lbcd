/*
 * lbcd kernel code for Ultrix
 */

#include <stdio.h>
#include <sys/fixpoint.h>
#include <fcntl.h>
#include <nlist.h>
#include <time.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "lbcd.h"

#define C_KMEM "/dev/kmem"
#define C_VMUNIX "/vmunix"

static struct nlist nl[] = {
  { "_avenrun" },         /* load average */
#define K_NL_LOADAVERAGE 0
  { "_boottime" },         /* boottime */
#define K_NL_BOOTTIME 1
  { 0 }
};

static int kd = -1;
static int kernel_init = 0;

static int
kernel_open(void)
{
  if ( (kd = open(C_KMEM, O_RDONLY)) < 0 ){
    util_log_error("can't open %s: %%m",C_KMEM);
    exit(1);
  }

  if (nlist(C_VMUNIX,nl) < 0) {
    util_log_error("no namelist for %s",C_VMUNIX);
    exit(1);
  }

  kernel_init = 1;
  return 0;
}

static int
kernel_close(void)
{
  if (! kernel_init) return;
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

   stat=read(kd,dest,dest_len);
   if (stat==-1) {
     util_log_error("kernel read %s: %%m",C_KMEM);
     exit(1);
   }   
   return stat;
}

int
kernel_getload(double *l1, double *l5, double *l15)
{
  fix kern_avenrun[3];

  if (!kernel_init) kernel_open();

  if (nl[K_NL_LOADAVERAGE].n_type==0) return -1;

  if (kernel_read( nl[K_NL_LOADAVERAGE].n_value, 
                       (char *) kern_avenrun,sizeof(kern_avenrun))==-1) { 
    util_log_error("kernel loadaverage read error: %%m");
    return -1;
  }

  *l1  = FIX_TO_DBL(kern_avenrun[0]);  
  *l5  = FIX_TO_DBL(kern_avenrun[1]);  
  *l15 = FIX_TO_DBL(kern_avenrun[2]);  

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
