/*
 * lbcd client-server routines
 * Obtains and sends polling information.
 * Also acts as a test driver.
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "lbcd.h"

/*
 * Prototypes
 */
static void lbcd_set_load(P_LB_RESPONSE *lb, P_HEADER_FULLPTR ph);
static void lbcd_proto2_convert(P_LB_RESPONSE *lb);
extern void lbcd_print_load(void);

void
lbcd_set_load(P_LB_RESPONSE *lb, P_HEADER_FULLPTR ph)
{
  int i, numserv;

  /* Clear pad and set number of requested services */
  lb->pad = 0;
  lb->services = numserv = ph->h.status;

  /* Set default weight and convert to network byte order */
  lbcd_setweight(lb,0,"default");
  lb->weights[0].host_weight = htonl(lb->weights[0].host_weight);
  lb->weights[0].host_incr = htonl(lb->weights[0].host_incr);

  /* Set requested services, if any */
  for (i = 1; i <= numserv; i++) {
    /* Obtain values */
    lbcd_setweight(lb,i,ph->names[i]);

    /* Convert to network byte order */
    lb->weights[i].host_weight = htonl(lb->weights[i].host_weight);
    lb->weights[i].host_incr = htonl(lb->weights[i].host_incr);
  }
}

/*
 * Convert a protocol 3 packet to a form a protocol 2
 * server will use properly.
 *
 * Version 2 of the protocol had no mechanism for client-computed
 * load.  To work around this problem, we just lie to the version 2
 * server such that the weight lbnamed will compute will be the
 * weight we desire.  Note that we have no control over the
 * increment for version 2 lbnamed -- that value is hard-coded.
 */
void
lbcd_proto2_convert(P_LB_RESPONSE *lb)
{
  /* lbnamed v2 only used l1, tot_users, and uniq_users */
  lb->l1 = (u_short)lb->weights[0].host_weight;
  lb->tot_users = 0;
  lb->uniq_users = 0;
}

void
lbcd_pack_info(P_LB_RESPONSE *lb, P_HEADER_FULLPTR ph)
{
  double l1,l5,l15;
  time_t bt,ct;
  int tu,uu,oc;
  time_t umtime;

  /*
   * Timestamps
   */
  kernel_getboottime(&bt);
  lb->boot_time = htonl(bt);
  time(&ct);
  lb->current_time = htonl(ct);

  /*
   * Load
   */
  kernel_getload(&l1,&l5,&l15);
  lb->l1 = htons((u_short)(l1*100));
  lb->l5 = htons((u_short)(l5*100));
  lb->l15 = htons((u_short)(l15*100));

  get_user_stats(&tu,&uu,&oc,&umtime);

  lb->tot_users = htons(tu);
  lb->uniq_users = htons(uu);
  lb->on_console = oc;
  lb->user_mtime = htonl(umtime);

  /*
   * Additional Fields
   */
  lb->reserved = 0;
  lb->tmp_full = tmp_full("/tmp");
#ifdef P_tmpdir
  lb->tmpdir_full = tmp_full(P_tmpdir);
#else
  lb->tmpdir_full = lb->tmp_full;
#endif

  lbcd_set_load(lb,ph);

  /*
   * Backward compatibility
   */
  if (lb->h.version < 3) {
    lbcd_proto2_convert(lb);
  }
}

void
lbcd_test(int argc, char *argv[])
{
  P_LB_RESPONSE lb;
  P_HEADER_FULL ph;
  int i;

  /* Create query packet */
  lb.h.version = ph.h.version = LBCD_VERSION;
  lb.h.id      = ph.h.id = 0;
  lb.h.op      = ph.h.op = op_lb_info_req;
  lb.h.status  = ph.h.status = 0;

  /* Fill reply */
  lbcd_pack_info(&lb,&ph);

  /* Print results */
  printf("PROTOCOL %d\n",lb.h.version);
  printf("\n");
  printf("MACHINE STATUS:\n");
  printf("l1           = %d\n",  ntohs(lb.l1));
  printf("l5           = %d\n",  ntohs(lb.l5));
  printf("l15          = %d\n",  ntohs(lb.l15));
  printf("current_time = %d\n",  (int)ntohl(lb.current_time));
  printf("boot_time    = %d\n",  (int)ntohl(lb.boot_time));
  printf("user_mtime   = %d\n",  (int)ntohl(lb.user_mtime));
  printf("tot_users    = %d\n",  ntohs(lb.tot_users));
  printf("uniq_users   = %d\n",  ntohs(lb.uniq_users));
  printf("on_console   = %d\n",  lb.on_console);
  printf("tmp_full     = %d\n",  lb.tmp_full);
  printf("tmpdir_full  = %d\n",  lb.tmpdir_full);
  printf("\n");
  printf("SERVICES: %d\n",lb.services);

  for (i = 0; i <= lb.services; i++) {
    printf("%d: weight %8d increment %8d\n",i,
	   (int)ntohl(lb.weights[i].host_weight),
	   (int)ntohl(lb.weights[i].host_incr));
  }

  exit(0);
}
