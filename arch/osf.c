/*
 * lbcd kernel code for Digital UNIX
 */

#include <stdio.h>
#include<sys/types.h>
#include<sys/table.h>
#include <fcntl.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "lbcd.h"
#include "util.h"

int
kernel_getload(double *l1, double *l5, double *l15)
{
  struct tbl_loadavg load;
  struct tbl_sysinfo sys;
  struct utmp *ut;

  if (table(TBL_LOADAVG, 0, &load, 1, sizeof(load))==-1) {
    return -1;
  }

  if (load.tl_lscale) {
         *l1 = load.tl_avenrun.l[0]/(float)(load.tl_lscale);
         *l5 = load.tl_avenrun.l[1]/(float)(load.tl_lscale);
         *l15 = load.tl_avenrun.l[2]/(float)(load.tl_lscale);
  } else {
         *l1 = load.tl_avenrun.d[0];
         *l5 = load.tl_avenrun.d[1];
         *l15 = load.tl_avenrun.d[2];
  }
  return 0;
}

int
kernel_getboottime(time_t *boottime)
{
  struct tbl_sysinfo sys;

  if (table(TBL_SYSINFO, 0, &sys, 1, sizeof(sys))==-1) return -1;

  *boottime = sys.si_boottime;

  return 0;
}

#ifdef MAIN
int
main()
{
  double l1,l5,l15;
  time_t boottime;

  if (kernel_getload(&l1,&l5,&l15) == 0) {
    printf("load %.02f %.02f %.02f\n",l1,l5,15);
  }
  if (kernel_getboottime(&boottime) == 0) {
    printf("booted at %s",ctime(&boottime));
  }

  return 0;
}
#endif
