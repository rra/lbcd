#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

extern int tcp_connect (char *host, char *protocol, int port);
extern int lbcd_check_reply(int sd, int timeout, char *token);

int
probe_tcp(char *host,
	  char *service, short port,
	  char *replycheck,
	  int timeout) 
{
  int sd;
  int retval = 0;

  if ((sd = tcp_connect(host ? host : "localhost",service,port)) == -1) {
    return -1;
  } else {
    if (replycheck) {
      retval = lbcd_check_reply(sd,timeout,replycheck);
      write(sd,"quit\r\n",6);
    }
    close(sd);
  }

  return retval;
}

int
lbcd_tcp_weight(u_int *weight_val, u_int *incr_val,
		int timeout, char *portarg)
{
  char *service = NULL;
  short port = 0;
  char *cp;

  /* Parse portarg */
  if (portarg == NULL)		/* we need to connect to something */
    return -1;

  for (cp = portarg; *cp; cp++) {
    if (!isdigit(*cp)) {
      service = portarg;	/* must be a port name */
      break;
    }
  }
  if (!*cp) {			/* must be a port number */
    port = (short)atoi(portarg);
    if (port < 1)		/* port must be positive short */
      return -1;
  }

  /* Connect to the port, set weight, and return */
  return *weight_val = (u_int)probe_tcp("localhost",service,
					port,NULL,timeout);
}

#ifdef TCPMAIN
int
main(int argc, char *argv[])
{
  int status;
  u_int i;

  status = probe_tcp(argv[1],"smtp",25,"220",5);
  printf("%s service %savailable\n","smtp",status ? "not " : "");
  if (lbcd_tcp_weight(&i,&i,5,"smtp") == 0) {
    printf("service available\n");
  } else {
    printf("service not available\n");
  }

  return status;
}
#endif
