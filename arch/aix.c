/*
 * lbcd kernel code for AIX
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>
#include <fcntl.h>
#include <time.h>
#include <nlist.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "lbcd.h"

#define C_KMEM "/dev/kmem"
#define C_VMUNIX "/unix"
#define FSCALE 65536.0

static struct nlist nl[] = {
  { "avenrun",0,0,0,0,0 },         /* load average */
#define K_NL_LOADAVERAGE 0
  { NULL,0,0,0,0,0 },  
};

static int kd = -1;
static int kernel_init=0;

static int
kernel_open()
{
  if ( (kd = open(C_KMEM, O_RDONLY)) < 0 ){
    util_log_error("can't open %s: %%m",C_KMEM);
    exit(1);
  }

  if (knlist(nl, 1, sizeof(struct nlist)) == -1) {
    util_log_error("no namelist for %s",C_VMUNIX);
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
  int kern_avenrun[3];

  if (!kernel_init) kernel_open();

  if (kernel_read( nl[K_NL_LOADAVERAGE].n_value, 
                       (char *) kern_avenrun,sizeof(kern_avenrun))==-1) { 
     util_log_error("kernel loadaverage read error: %%m");
      return -1;
  }

  *l1  = kern_avenrun[0]/FSCALE;
  *l5  = kern_avenrun[1]/FSCALE;
  *l15 = kern_avenrun[2]/FSCALE;

  return 0;
}

int
kernel_getboottime(time_t *boottime)
{
  time_t uptime,curr;
  struct tms tbuf;

  uptime = times(&tbuf) / HZ;
  curr = time((time_t*)0);
  *boottime = curr-uptime;

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
