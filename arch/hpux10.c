/*
 * lbcd kernel code for HP-UX 10.x
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/time.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/param.h>
#include <sys/pstat.h>
#include <utmp.h>
#include <utmpx.h>
#include <sys/time.h>
#include <sys/types.h>

int
kernel_getload(double *l1, double *l5, double *l15)
{
  struct pst_dynamic dyn_info;

  if (pstat_getdynamic (&dyn_info, sizeof (dyn_info), 0, 0) < 0)
    return -1;

  *l1  = dyn_info.psd_avg_1_min;
  *l5  = dyn_info.psd_avg_5_min;
  *l15 = dyn_info.psd_avg_15_min;

  return 0;
}

int
kernel_getboottime(time_t *boottime)
{
  int fd;

  if ((fd = open(UTMPX_FILE,O_RDONLY|O_NONBLOCK)) != -1) {
    int nread;
    struct utmpx ut;

    while ((nread = read(fd,&ut,sizeof(ut))) > 0) {
      if (strcmp(BOOT_MSG,ut.ut_line) == 0) {
	*boottime = ut.ut_tv.tv_sec;
	break;
      }
    }
    close(fd);
  }

  return *boottime == 0;
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
