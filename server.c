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

int
lbcd_recv_udp(int s, 
	      struct sockaddr_in *cli_addr, int * cli_len,
	      char *mesg, int max_mesg)
{
  int n;
  P_HEADER_PTR ph;

  n = recvfrom(s,mesg, max_mesg, 0, (struct sockaddr *)cli_addr,cli_len);

  if (n < 0) {
    util_log_error("recvfrom: %%m");
    exit(1);
  }

  if (n < sizeof(P_HEADER)) {
    util_log_error("short packet received, len %d",n);
    return 0;
  }
  ph = (P_HEADER_PTR) mesg;     
  ph->version = ntohs(ph->version);
  ph->id      = ntohs(ph->id);
  ph->op      = ntohs(ph->op);
  ph->status  = ntohs(ph->status);

  switch(ph->version) {
  case 3:
    /* Client-supplied load protocol */
    break;
  case 2:
    /* Original protocol */
    break;
  default:
    util_log_error("protocol version unsupported: %d", ph->version);
    lbcd_send_status(s, cli_addr, *cli_len,ph,status_lbcd_version);
    return 0;
  }

  if (ph -> status != status_request) {
    util_log_error("expecting request, got %d",ph->status);
    lbcd_send_status(s, cli_addr, *cli_len,ph,status_lbcd_error);
    return 0;
  }
  
  return n;     
}

void
lbcd_set_load(P_LB_RESPONSE *lb)
{
  /* FIXME: fill in below */
#if 1
  lb->services = 0;
  lb->host_weight = 0;
  lb->host_incr = 0;
#else
  int i;
  set_load(lb,0);
  for (i = 1; i < lb->services; i++) {
    set_load(lb,i);
  }
#endif
}

void
lbcd_pack_info(P_LB_RESPONSE *lb, int round_robin)
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

  /* Client-Computed load for Version 2 lbnamed
   *
   * Version 2 of the protocol had no mechanism for client-computed
   * load.  To work around this problem, we just lie to the version 2
   * server such that the weight lbnamed will compute will be the
   * weight we desire.  Note that we have no control over the
   * increment for version 2 lbnamed -- that value is hard-coded.
   */
  if (round_robin && lb->h.version < 3) {
    lb->l1 = 100;
    lb->tot_users = 0;
    lb->uniq_users = 0;
    lb->on_console = 0;
  }

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
}

int
lbcd_send_status(int s, 
		 struct sockaddr_in *cli_addr, int cli_len,
		 P_HEADER *request_header,
		 p_status_t pstat)
{
  P_HEADER header;
  header.version= htons(LBCD_VERSION);
  header.id     = htons(request_header->id);
  header.op     = htons(request_header->op);
  header.status = htons(pstat);

  if (sendto(s,(char *)&header,sizeof(header),0,
	     (struct sockaddr *)cli_addr,cli_len)!=sizeof(header)) {
    util_log_error("sendto: %%m");
    return -1;
  }
  return 0;
}

void
lbcd_print_load(void)
{
  P_LB_RESPONSE lb;

  lbcd_pack_info(&lb,0);

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
