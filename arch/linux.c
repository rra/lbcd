/*
 * lbcd kernel code for Linux
 */

#include <stdio.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "lbcd.h"
#include "util.h"

int
kernel_getload(double *l1, double *l5, double *l15)
{
  FILE *fp;

  if ((fp = fopen ("/proc/loadavg", "r")) == (FILE *)0) {
    util_log_error("cannot open /proc/loadavg");
  }

  fscanf(fp, "%lf %lf %lf", l1,l5,l15);
  fclose(fp);

  return 0;
}

int
kernel_getboottime(time_t *boottime)
{
  FILE *fp;
  double uptime;
  time_t curr;

  if ((fp = fopen ("/proc/uptime", "r")) == (FILE *)0) {
    util_log_error("cannot open /proc/uptime");
  }

  fscanf(fp, "%lf", &uptime);
  fclose(fp);

  curr = time((time_t *)0);
  *boottime = curr - uptime;

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
