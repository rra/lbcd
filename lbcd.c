
#include "config.h"
#include "protocol.h"

#include <signal.h>

#include <errno.h>
extern char *sys_errlist[];
extern int errno;


void handle_lb_request(int s,P_HEADER_PTR ph,  int ph_len,
                     struct sockaddr_in *cli_addr, int cli_len);
void usage();
void handle_requests();

extern int z_flag;

int d_flag = 0;
int p_flag = 0;
int r_flag = 0;
int s_flag = 0;
int l_flag = 0;
int z_flag = 0;

int err_flag = 0;

char *pid_flag;

char *pid_file=PID_FILE;

int
main(int argc, char **argv)
{
   int c;
   extern int optind, opterr;
   extern char *optarg;
   int pid;
 
   while ((c = getopt(argc, argv, "P:dlp:rsz")) != EOF) 
     switch (c) {
     case 'P':
         pid_file=optarg;
         break;
     case 'd':
         d_flag=1;
         break;
     case 'l':
         l_flag=1;
         break;
     case 'p':
         p_flag = atoi(optarg);
         break;
     case 'r':
         r_flag=1;
         break;
     case 's':
         s_flag=1;
         break;
     case 'z':
         z_flag=1;
	 break;
     default:
         err_flag++;
     }

  if (err_flag) (void)usage();

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
            handle_lb_request(s,ph,n,&cli_addr,cli_len);
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
		  struct sockaddr_in *cli_addr, int cli_len)
{
   P_LB_RESPONSE lbr;

   lbr.h.version = htons(ph->version);
   lbr.h.id      = htons(ph->id);
   lbr.h.op      = htons(ph->op);
   lbr.h.status  = htons(status_ok);

   proto_pack_lb_info( &lbr);

   if (sendto(s,(const char *)&lbr,sizeof(lbr),
                0,(const struct sockaddr *)cli_addr,cli_len)!=sizeof(lbr)) {
     util_log_error("sendto: %%m");
   }

}

void
usage()
{
  fprintf(stderr,"Usage:   %s [options] [-d] [-p port]\n",PROGNAME);
  fprintf(stderr,"   -d          debug mode, don't fork off\n");
  fprintf(stderr,"   -l          log various requests\n");
  fprintf(stderr,"   -p port     run using different port number\n");
  fprintf(stderr,"   -r          restart (kill current lbcd)\n");
  fprintf(stderr,"   -s          stop running lbcd\n");
  fprintf(stderr,"   -z          always report zero load\n");
  fprintf(stderr,"   -P file     pid file\n");
  exit(1);
}

