/*
 * lbcd kernel code for Solaris
 *
 * Uses kstat code in the Solaris FAQ written by Casper Dik
 * in the comp.unix.solaris post <3qh88s$6ho@engnews2.Eng.Sun.COM>.
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/times.h>
#include <limits.h>
#include <kstat.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <utmp.h>
#include <utmpx.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static int kernel_init=0;
static kstat_ctl_t *kc;

int
kernel_open()
{
  if ((kc = kstat_open()) == 0) { }

  kernel_init = 1;
  return 0;
}

int
kernel_close()
{
  if (! kernel_init) return 0;

  return kstat_close(kc);
}

int
kernel_getload(double *l1, double *l5, double *l15)
{
  kstat_t *ksp;
  kstat_named_t *kn1,*kn5,*kn15;

  if (!kernel_init) kernel_open();

  if ((ksp = kstat_lookup(kc, "unix", 0, "system_misc")) == 0 ) {

    return -1;
  }

  if (kstat_read(kc, ksp, 0) == -1) {

    return -1;
  }

  if ((kn1 = kstat_data_lookup(ksp, "avenrun_1min")) == 0) {

    return -1;
  }
  if ((kn5 = kstat_data_lookup(ksp, "avenrun_5min")) == 0) {

    return -1;
  }
  if ((kn15 = kstat_data_lookup(ksp, "avenrun_15min")) == 0) {

    return -1;
  }

  *l1  = (double)kn1->value.ul/FSCALE;
  *l5  = (double)kn5->value.ul/FSCALE,
  *l15 = (double)kn15->value.ul/FSCALE;

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
