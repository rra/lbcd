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
#include "protocol.h"
#include "lbcd.h"
#include "util.h"

extern char *sys_errlist[];
extern int errno;

static void handle_lb_request(int s,P_HEADER_PTR ph,  int ph_len,
			      struct sockaddr_in *cli_addr, int cli_len,
			      int rr);
static void usage(void);
static void handle_requests(void);

static int d_flag = 0;
static int p_flag = 0;
static int r_flag = 0;
static int s_flag = 0;
static int l_flag = 0;
static int R_flag = 0;
static int t_flag = 0;
static int err_flag = 0;
static char *pid_flag;
static char *pid_file = PID_FILE;
static char *lbcd_helper = (char *)0;

int
main(int argc, char **argv)
{
   int c;
   extern int optind, opterr;
   extern char *optarg;
   int pid;
 
   while ((c = getopt(argc, argv, "P:Rc:dlp:rstz")) != EOF) 
     switch (c) {
     case 'P': /* pid file */
         pid_file=optarg;
         break;
     case 'R': /* round-robin */
         R_flag=1;
	 break;
     case 'c': /* helper command -- must be full path to command */
         lbcd_helper = optarg;
	 if (access(lbcd_helper,X_OK) != 0) {
	   fprintf(stderr,"optarg: no such program\n");
	   exit(1);
	 }
         break;
     case 'd': /* debugging mode */
         d_flag=1;
         break;
     case 'l':
         l_flag=1;
         break;
     case 'p': /* port number */
         p_flag = atoi(optarg);
         break;
     case 'r': /* restart */
         r_flag=1;
         break;
     case 's': /* stop */
         s_flag=1;
         break;
     case 't': /* test mode */
         t_flag=1;
         break;
     default:
         err_flag++;
     }

  if (err_flag)
    usage();

  if (!p_flag) p_flag = PROTO_PORTNUM;

  if (s_flag) {
     pid = util_get_pid_from_file(pid_file);
     if (pid == -1) {
         fprintf(stderr,"no lbcd running");
         exit(1);
     }
     if (kill(pid,SIGTERM)==-1) {
         fprintf(stderr,"kill: %s\n",sys_errlist[errno]);
         exit(1);
     }
     exit(0);
  }

  if (!d_flag) util_start_daemon();

  handle_requests();
}

void
handle_requests()
{
   int s;
   struct sockaddr_in serv_addr, cli_addr;
   int cli_len;
   int n; 
   char mesg[PROTO_MAXMESG];
   P_HEADER_PTR ph;

  /* open UDP socket */
  
  if (  (s = socket(AF_INET,SOCK_DGRAM, 0)) < 0 ) {
     util_log_error("can't open udp socket: %%m");
     exit(1);
  }

  memset(& serv_addr, 0, sizeof(serv_addr));  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(p_flag);
  
  while (bind(s,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
     if(errno==EADDRINUSE) {
        if(r_flag) {
           int pid;
           pid = util_get_pid_from_file(pid_file);
           if (kill(pid,SIGTERM)==-1) {
              util_log_error("can't kill pid %d: %%m",pid);
              exit(1);
           } 
           r_flag=0; /* don't loop forever */
        } else {
              util_log_error("lbcd already running?");
              exit(1);
	}
     } else {
        util_log_error("can't bind udp socket: %%m");
        exit(1);
    }
   }

  util_write_pid_in_file(pid_file);

  util_log_info("ready to accept requests");
  
  while(1) {

    cli_len = sizeof(cli_addr);

    n=proto_recv_udp(s, (struct sockaddr *)&cli_addr, &cli_len, 
                                              mesg, sizeof(mesg));

    if (n>0) {
         ph = (P_HEADER_PTR) mesg;
         switch (ph->op) {
         case op_lb_info_req: 
            handle_lb_request(s,ph,n,&cli_addr,cli_len,R_flag);
            break;
	 default:
            proto_send_status(s, &cli_addr, cli_len,ph,status_unknown_op);
            util_log_error("unknown op requested: %d",ph->op);
	 }
    }     
  }

}

void
handle_lb_request(int s,P_HEADER_PTR ph, int ph_len,
		  struct sockaddr_in *cli_addr, int cli_len,
		  int rr)
{
   P_LB_RESPONSE lbr;

   lbr.h.version = htons(ph->version);
   lbr.h.id      = htons(ph->id);
   lbr.h.op      = htons(ph->op);
   lbr.h.status  = htons(status_ok);

   proto_pack_lb_info(&lbr,rr);

   if (sendto(s,(const char *)&lbr,sizeof(lbr),
                0,(const struct sockaddr *)cli_addr,cli_len)!=sizeof(lbr)) {
     util_log_error("sendto: %%m");
   }

}

void
usage(void)
{
  fprintf(stderr,"Usage:   %s [options] [-d] [-p port]\n",PROGNAME);
  fprintf(stderr,"   -c cmd      run cmd (full path) to obtain load values\n");
  fprintf(stderr,"   -d          debug mode, don't fork off\n");
  fprintf(stderr,"   -l          log various requests\n");
  fprintf(stderr,"   -p port     run using different port number\n");
  fprintf(stderr,"   -r          restart (kill current lbcd)\n");
  fprintf(stderr,"   -R          round-robin polling\n");
  fprintf(stderr,"   -s          stop running lbcd\n");
  fprintf(stderr,"   -t          test mode (print stats and exit)\n");
  fprintf(stderr,"   -P file     pid file\n");
  exit(1);
}

