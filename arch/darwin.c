/*
 * lbcd kernel code for Mac OS Darwin
 * 
 * Ported to Darwin by Clif Redding-11/15/07
 * Computer Science Dept., University of West Florida
 *
 * It uses the sysctl function to get the required information
 * from the kernel
 * Works on Darwin 8.10.1 Mac OS X 10.4.10
 * I make no guarantee or warranty that it works with anything else.
 */

#include "config.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysctl.h>

int
kernel_getload(double *l1, double *l5, double *l15)
{
  int mib[2];
  struct loadavg dyn_info;
  size_t sdi;

  mib[0]=CTL_VM;
  mib[1]=VM_LOADAVG;
  sdi = sizeof(dyn_info);
  if (sysctl( mib,2,&dyn_info,&sdi,NULL,0)==-1) { 
    util_log_error("kernel loadaverage read error: %%m");
    return -1;
  }

  *l1  = (double)dyn_info.ldavg[0]/FSCALE;
  *l5  = (double)dyn_info.ldavg[1]/FSCALE;
  *l15 = (double)dyn_info.ldavg[2]/FSCALE;

  return 0;
}

int
kernel_getboottime(time_t *boottime)
{
  int mib[2];
  struct timeval boot;
  size_t sb;

  mib[0]=CTL_KERN;
  mib[1]=KERN_BOOTTIME;
  sb = sizeof(boot);
  if (sysctl( mib,2,&boot,&sb,NULL,0)==-1) {
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

