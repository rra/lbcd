/*
 * lbcd client main
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "lbcd.h"

/*
 * Prototypes
 */
static int stop_lbcd(const char *pid_file);
static void usage(int exitcode);

static void handle_requests(int port,
			    const char *pid_file,
			    const char *lbcd_helper);
static void handle_lb_request(int s,
			      P_HEADER_PTR ph,
			      int ph_len,
			      struct sockaddr_in *cli_addr,
			      int cli_len);
static int lbcd_recv_udp(int s, struct sockaddr_in *cli_addr,
			 int cli_len, char *mesg, int max_mesg);
static int lbcd_send_status(int s, struct sockaddr_in *cli_addr,
			    int cli_len,
			    P_HEADER *request_header,
			    p_status_t pstat);

int
main(int argc, char **argv)
{
  int debug = 0;
  int port = LBCD_PORTNUM;
  int testmode = 0;
  char *pid_file = PID_FILE;
  char *lbcd_helper = (char *)0;
  char *service_weight = (char *)0;
  int c;

  opterr = 1;
  while ((c = getopt(argc, argv, "P:Rc:dhlp:rstw:")) != EOF) {
    switch (c) {
    case 'h': /* usage */
      usage(0);
      break;
    case 'P': /* pid file */
      pid_file = optarg;
      break;
    case 'R': /* round-robin */
      service_weight = "rr";
      break;
    case 'c': /* helper command -- must be full path to command */
      lbcd_helper = optarg;
      if (access(lbcd_helper,X_OK) != 0) {
	fprintf(stderr,"%s: no such program\n",optarg);
	exit(1);
      }
      break;
    case 'd': /* debugging mode */
      debug = 1;
      break;
    case 'l': /* log requests */
      /* FIXME: implement */
      break;
    case 'p': /* port number */
      port = atoi(optarg);
      break;
    case 'r': /* restart */
      if (stop_lbcd(pid_file) != 0) {
	exit(1);
      }
      break;
    case 's': /* stop */
      exit(stop_lbcd(pid_file));
      break;
    case 't': /* test mode */
      testmode = 1;
      break;
    case 'w': /* weight or service */
      service_weight = optarg;
      break;
    default:
      usage(1);
      break;
    }
  }

  /* Initialize default load handler */
  if (lbcd_weight_init(lbcd_helper,service_weight) != 0) {
    fprintf(stderr,"could not initialize service handler\n");
    exit(1);
  }

  /* If testing, print default output and terminate */
#if 0
  if (testmode) {
  }
#endif

  /* Fork unless debugging */
  if (!debug)
    util_start_daemon();

  /* Become a daemon.  handle_requests never returns */
  handle_requests(port,pid_file,lbcd_helper);
  return 0;
}

int
stop_lbcd(const char *pid_file)
{
  pid_t pid;

  if ((pid = util_get_pid_from_file(pid_file)) == -1) {
    return 0;
  }
  if (kill(pid,SIGTERM) == -1) {
    util_log_error("can't kill pid %d: %%m",pid);
    perror("kill");
    return -1;
  }
  return 0;
}

void
handle_requests(int port, const char *pid_file,
		const char *lbcd_helper)
{
   int s;
   struct sockaddr_in serv_addr, cli_addr;
   int cli_len;
   int n; 
   char mesg[LBCD_MAXMESG];
   P_HEADER_PTR ph;

  /* open UDP socket */
  if ((s = socket(AF_INET,SOCK_DGRAM, 0)) < 0) {
     util_log_error("can't open udp socket: %%m");
     exit(1);
  }

  memset(& serv_addr, 0, sizeof(serv_addr));  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);
  
  if (bind(s,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    if(errno==EADDRINUSE) {
      util_log_error("lbcd already running?");
    } else {
      util_log_error("cannot bind udp socket: %%m");
    }
    exit(1);
  }

  util_write_pid_in_file(pid_file);
  util_log_info("ready to accept requests");

  /* Main loop */
  while(1) {
    cli_len = sizeof(cli_addr);
    n = lbcd_recv_udp(s,&cli_addr,cli_len,mesg,sizeof(mesg));
    if (n>0) {
      ph = (P_HEADER_PTR) mesg;
      switch (ph->op) {
      case op_lb_info_req: 
	handle_lb_request(s,ph,n,&cli_addr,cli_len);
	break;
      default:
	lbcd_send_status(s,&cli_addr,cli_len,ph,status_unknown_op);
	util_log_error("unknown op requested: %d",ph->op);
      }
    }
  }
}

void
handle_lb_request(int s,
		  P_HEADER_PTR ph, int ph_len,
		  struct sockaddr_in *cli_addr, int cli_len)
{
   P_LB_RESPONSE lbr;
   int pkt_size;

   /* Fill in reply header */
   lbr.h.version = htons(ph->version);
   lbr.h.id      = htons(ph->id);
   lbr.h.op      = htons(ph->op);
   lbr.h.status  = htons(status_ok);

   /* Fill in reply */
   lbcd_pack_info(&lbr,ph);

   /* Send reply */
   pkt_size = sizeof(lbr) + lbr.services * sizeof(LBCD_SERVICE);
   if (sendto(s,(const char *)&lbr, sizeof(lbr), 0,
	      (const struct sockaddr *)cli_addr, cli_len)
	      != pkt_size) {
     util_log_error("sendto: %%m");
   }
}

int
lbcd_recv_udp(int s, 
	      struct sockaddr_in *cli_addr, int cli_len,
	      char *mesg, int max_mesg)
{
  int n;
  P_HEADER_PTR ph;

  n = recvfrom(s,mesg, max_mesg, 0,
	       (struct sockaddr *)cli_addr,&cli_len);

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
  case 3: /* Client-supplied load protocol */
    /* Extended services request: status > 0 */
    if (ph->status > LBCD_MAX_SERVICES) {
      ph->status = LBCD_MAX_SERVICES; /* trim query to max allowed */
    }
    break;
  case 2: /* Original protocol */
    break;
  default:
    util_log_error("protocol version unsupported: %d", ph->version);
    lbcd_send_status(s, cli_addr, cli_len, ph, status_lbcd_version);
    return 0;
  }

  if (ph -> status != status_request) {
    util_log_error("expecting request, got %d",ph->status);
    lbcd_send_status(s, cli_addr, cli_len, ph, status_lbcd_error);
    return 0;
  }
  
  return n;     
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
usage(int exitcode)
{
  fprintf(stderr,"Usage:   %s [options] [-d] [-p port]\n",PROGNAME);
  fprintf(stderr,"   -h          print usage\n");
  fprintf(stderr,"   -c cmd      run cmd (full path) to obtain load values\n");
  fprintf(stderr,"   -d          debug mode, don't fork off\n");
  fprintf(stderr,"   -l          log various requests\n");
  fprintf(stderr,"   -p port     run using different port number\n");
  fprintf(stderr,"   -r          restart (kill current lbcd)\n");
  fprintf(stderr,"   -R          round-robin polling\n");
  fprintf(stderr,"   -s          stop running lbcd\n");
  fprintf(stderr,"   -w option   specify returned weight; options:\n");
  fprintf(stderr,"               either \"load[:incr]\" or \"service\"\n");
  fprintf(stderr,"   -t          test mode (print stats and exit)\n");
  fprintf(stderr,"   -P file     pid file\n");
  exit(exitcode);
}

