/*
 * lbcd client-server routines
 * Obtains and sends polling information.
 * Also acts as a test driver.
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "lbcd.h"

void
lbcd_set_load(P_LB_RESPONSE *lb)
{
#if 1
  lbcd_default_weight(lb,&lb->host_weight,&lb->host_incr);

  /* FIXME: fill in below */
  lb->pad = 0;
  lb->services = 0;
#else
  int i;
  for (i = 1; i < lb->services; i++) {
    set_load(lb,i);
  }
#endif
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
  lb->l1 = lb->host_weight;
  lb->tot_users = 0;
  lb->uniq_users = 0;
}

void
lbcd_pack_info(P_LB_RESPONSE *lb, int round_robin, P_HEADER_PTR ph)
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
  lb->user_mtime = htonl(umtime);

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

  lbcd_set_load(lb);

  /*
   * Backward compatibility
   */
  if (lb->h.version < 3) {
    lbcd_proto2_convert(lb);
  }
}

void
lbcd_print_load(void)
{
  P_LB_RESPONSE lb;

  lbcd_pack_info(&lb,0,0);

  printf("  lb.l1 = %d\n",  ntohs(lb.l1));
  printf("  lb.l5 = %d\n",  ntohs(lb.l5));
  printf("  lb.l15 = %d\n",  ntohs(lb.l15));
  printf("  lb.current_time = %ld\n",  ntohl(lb.current_time));
  printf("  lb.boot_time = %ld\n",  ntohl(lb.boot_time));
  printf("  lb.user_mtime = %ld\n",  ntohl(lb.user_mtime));
  printf("  lb.tot_users = %d\n",  ntohs(lb.tot_users));
  printf("  lb.uniq_users = %d\n",  ntohs(lb.uniq_users));
  printf("  lb.on_console = %d\n",  lb.on_console);
  printf("  lb.tmp_full = %d\n",  lb.tmp_full);
  printf("  lb.tmpdir_full = %d\n",  lb.tmpdir_full);
}

#ifdef MAIN
int
main(int argc, char *argv[])
{
  lbcd_print_load();
  return 0;
}
#endif
