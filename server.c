/*
 * lbcd client-server routines
 * Obtains and sends polling information.
 * Also acts as a test driver.
 */

#include "config.h"
#include "lbcd.h"
#include "proto_server.h"

int
proto_recv_udp(int s, 
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
    /* Basic Extended Version */
    if (ph->status > 0) {
      util_log_error("extended protcol 3 not supported yet");
      proto_send_status(s, cli_addr, *cli_len,ph,status_proto_version);
    }
    break;
  case 2:
    /* Original Protocol */
    break;
  default:
    util_log_error("protocol version unsupported: %d", ph->version);
    proto_send_status(s, cli_addr, *cli_len,ph,status_proto_version);
    return 0;
  }

  if (ph -> status != status_request) {
    util_log_error("expecting request, got %d",ph->status);
    proto_send_status(s, cli_addr, *cli_len,ph,status_proto_error);
    return 0;
  }
  
  return n;     
}

void
proto_pack_lb_info(P_LB_RESPONSE *lb)
{
  double l1,l5,l15;
  time_t bt,ct;
  int tu,uu,oc;
  time_t umtime;

  kernel_getboottime(&bt);
  lb->boot_time = htonl(bt);

  time(&ct);
  lb->current_time = htonl(ct);

  kernel_getload(&l1,&l5,&l15);
  lb->l1 = htons((u_short)(l1*100));
  lb->l5 = htons((u_short)(l5*100));
  lb->l15 = htons((u_short)(l15*100));

  get_user_stats(&tu,&uu,&oc,&umtime);

  lb->tot_users = htons(tu);  
  lb->uniq_users = htons(uu);
  lb->on_console = oc;
  lb->user_mtime = htonl(umtime);
  lb->reserved = 0;
  lb->tmp_full = tmp_full();
  /* FIXME: fill in below */
  lb->tmpdir_full = tmp_full();
  lb->pad = htons(0);
  lb->host_weight = htonl(0);
  lb->host_incr = htonl(0);
}

int
proto_send_status(int s, 
		  struct sockaddr_in *cli_addr, int cli_len,
		  P_HEADER *request_header,
		  p_status_t pstat)
{
  P_HEADER header;
  header.version= htons(PROTO_VERSION);
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

#ifdef MAIN

int
main(int argc, char *argv[])
{
  P_LB_RESPONSE lb;

  proto_pack_lb_info(&lb);

  printf("  lb.l1 = %d\n",  ntohs(lb.l1));
  printf("  lb.l5 = %d\n",  ntohs(lb.l5));
  printf("  lb.l15 = %d\n",  ntohs(lb.l15));
  printf("  lb.current_time = %d\n",  ntohl(lb.current_time));
  printf("  lb.boot_time = %d\n",  ntohl(lb.boot_time));
  printf("  lb.user_mtime = %d\n",  ntohl(lb.user_mtime));
  printf("  lb.tot_users = %d\n",  ntohs(lb.tot_users));
  printf("  lb.uniq_users = %d\n",  ntohs(lb.uniq_users));
  printf("  lb.on_console = %d\n",  lb.on_console);
  printf("  lb.tmp_full = %d\n",  lb.tmp_full);
}

#endif
