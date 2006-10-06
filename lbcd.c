/*
 * lbcd client main
 */

#include "version.h"

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
static void version(void);

static void handle_requests(int port,
			    const char *pid_file,
			    const char *lbcd_helper,
			    struct in_addr *bind_address,
			    int simple);
static void handle_lb_request(int s,
			      P_HEADER_FULLPTR ph,
			      int ph_len,
			      struct sockaddr_in *cli_addr,
			      int cli_len,
			      int simple);
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
  int restart = 0;
  int simple = 0;
  char *pid_file = PID_FILE;
  char *lbcd_helper = (char *)0;
  char *service_weight = (char *)0;
  struct in_addr bind_address;
  int service_timeout = LBCD_TIMEOUT;
  int c;

  /* A quick hack to honor --help and --version */
  if (argv[1])
    if (argv[1][0] == '-' && argv[1][1] == '-' && argv[1][2] != '\0') {
      switch(argv[1][2]) {
      case 'h':
	usage(0);
      case 'v':
	version();
      default:
	usage(1);
      }
    }

  opterr = 1;
  bind_address.s_addr = htonl(INADDR_ANY);
  while ((c = getopt(argc, argv, "P:Rb:c:dhlp:rSstT:w:")) != EOF) {
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
    case 'b': /* bind address */
      if (inet_aton(optarg, &bind_address) == 0) {
        fprintf(stderr,"invalid bind address %s",optarg);
        exit(1);
      }
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
      restart = 1;
      break;
    case 'S': /* simple, no version two adjustments */
      simple = 1;
      break;
    case 's': /* stop */
      exit(stop_lbcd(pid_file));
      break;
    case 't': /* test mode */
      testmode = 1;
      break;
    case 'T': /* timeout */
      service_timeout = atoi(optarg);
      if (service_timeout < 1 || service_timeout > 300) {
	fprintf(stderr,"timeout (%d) must be between 1 and 300 seconds\n",
		service_timeout);
	exit(1);
      }
      break;
    case 'w': /* weight or service */
      service_weight = optarg;
      break;
    default:
      usage(1);
      break;
    }
  }

  /* Handle restart, if requested */
  if (!testmode && restart && stop_lbcd(pid_file) != 0) {
    exit(1);
  }

  /* Initialize default load handler */
  if (lbcd_weight_init(lbcd_helper,service_weight,service_timeout) != 0) {
    fprintf(stderr,"could not initialize service handler\n");
    exit(1);
  }

  /* If testing, print default output and terminate */
  if (testmode) {
    lbcd_test(argc-optind,argv+optind);
  }

  /* Fork unless debugging */
  if (!debug)
    util_start_daemon();

  /* Become a daemon.  handle_requests never returns */
  handle_requests(port,pid_file,lbcd_helper,&bind_address,simple);
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
handle_requests(int port, const char *pid_file, const char *lbcd_helper,
		struct in_addr *bind_address, int simple)
{
   int s;
   struct sockaddr_in serv_addr, cli_addr;
   int cli_len;
   int n; 
   char mesg[LBCD_MAXMESG];
   P_HEADER_FULLPTR ph;

  /* open UDP socket */
  if ((s = socket(AF_INET,SOCK_DGRAM, 0)) < 0) {
     util_log_error("can't open udp socket: %%m");
     exit(1);
  }

  memset(& serv_addr, 0, sizeof(serv_addr));  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr = *bind_address;
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
      ph = (P_HEADER_FULLPTR) mesg;
      switch (ph->h.op) {
      case op_lb_info_req: 
	handle_lb_request(s,ph,n,&cli_addr,cli_len,simple);
	break;
      default:
	lbcd_send_status(s,&cli_addr,cli_len,&ph->h,status_unknown_op);
	util_log_error("unknown op requested: %d",ph->h.op);
      }
    }
  }
}

void
handle_lb_request(int s, P_HEADER_FULLPTR ph, int ph_len,
		  struct sockaddr_in *cli_addr, int cli_len, int simple)
{
   P_LB_RESPONSE lbr;
   int pkt_size;

   /* Fill in reply header */
   lbr.h.version = htons(ph->h.version);
   lbr.h.id      = htons(ph->h.id);
   lbr.h.op      = htons(ph->h.op);
   lbr.h.status  = htons(status_ok);

   /* Fill in reply */
   lbcd_pack_info(&lbr,ph,simple);

   /* Compute reply size (maximum packet minus unused service slots) */
   pkt_size = sizeof(lbr) -
     (LBCD_MAX_SERVICES - lbr.services) * sizeof(LBCD_SERVICE);

   /* Send reply */
   if (sendto(s,(const char *)&lbr, pkt_size, 0,
	      (const struct sockaddr *)cli_addr, cli_len)
       != pkt_size) {
     util_log_error("sendto: %%m");
   }
}

/*
 * lbcd_recv_udp
 *
 * Receive request packet and verify the integrity and format
 * of the reply.  This routine is REQUIRED to sanitize the request
 * packet.  All other program routines can expect that the packet
 * is safe to read once it is passed on.
 */
int
lbcd_recv_udp(int s, 
	      struct sockaddr_in *cli_addr, int cli_len,
	      char *mesg, int max_mesg)
{
  int n;
  P_HEADER_FULLPTR ph;

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

  /*
   * Convert request to host format
   */
  ph = (P_HEADER_FULLPTR) mesg;     
  ph->h.version = ntohs(ph->h.version);
  ph->h.id      = ntohs(ph->h.id);
  ph->h.op      = ntohs(ph->h.op);
  ph->h.status  = ntohs(ph->h.status);

  /*
   * Check protocol number and packet integrity
   */
  switch(ph->h.version) {
  case 3: /* Client-supplied load protocol */
    {
      int i;

      /* Extended services request: status > 0 */
      if (ph->h.status > LBCD_MAX_SERVICES) {
	ph->h.status = LBCD_MAX_SERVICES; /* trim query to max allowed */
      }
      /* Ensure NUL-termination of services name */
      for (i = 0; i < ph->h.status; i++) {
	ph->names[i][sizeof(LBCD_SERVICE_REQ)-1] = '\0';
      }

      break;
    }
  case 2: /* Original protocol */
    break;
  default:
    util_log_error("protocol version unsupported: %d", ph->h.version);
    lbcd_send_status(s, cli_addr, cli_len, &ph->h, status_lbcd_version);
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
  printf("Usage:   %s [options] [-d] [-p port]\n",PROGNAME);
  printf("   -h          print usage\n");
  printf("   -c cmd      run cmd (full path) to obtain load values\n");
  printf("   -d          debug mode, don't fork off\n");
  printf("   -l          log various requests\n");
  printf("   -p port     run using different port number\n");
  printf("   -r          restart (kill current lbcd)\n");
  printf("   -R          round-robin polling\n");
  printf("   -s          stop running lbcd\n");
  printf("   -w option   specify returned weight; options:\n");
  printf("               either \"load:incr\" or \"service\"\n");
  printf("   -t          test mode (print stats and exit)\n");
  printf("   -T seconds  timeout (1-300 seconds, default 5)\n");
  printf("   -P file     pid file\n");
  printf("   --help      print usage and exit\n");
  printf("   --version   print protocol version and exit\n");
  exit(exitcode);
}

void
version(void)
{
  printf("lbcd protocol %d version %s\n",LBCD_VERSION,VERSION);
  exit(0);
}
