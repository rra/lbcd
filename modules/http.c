#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

extern int tcp_connect (char *host, char *protocol, int port);

#define QUERY "GET / HTTP/1.0\r\n\r\n"

int
probe_http(char *host, int timeout, char *portarg)
{
  int sd;
  int retval = 0;
  short port = 80;		/* default port is 80 */
  char *service = "http";	/* default service name */

  if (portarg != NULL) {
    if ((port = (short)atoi(portarg)) < 1)
      port = 80;
    else
      service = NULL;		/* force supplied port number */
  }
  if ((sd = tcp_connect(host ? host : "localhost",service,port)) == -1) {
    return -1;
  } else {
    struct timeval tv = { timeout, 0 };
    fd_set rset;
    char buf[17];

    write(sd,QUERY,sizeof(QUERY));

    FD_ZERO(&rset);
    FD_SET(sd,&rset);
    if (select(sd+1, &rset, NULL, NULL, &tv) > 0) {
      buf[sizeof(buf)-1] = '\0';
      if (read(sd,buf,sizeof(buf)-1) > 0) {
	if (strstr(buf,"200") != NULL) {
	  retval = 0;
	} else {
	  retval = -1;
	}
      }
    } else {
      retval = -1;
    }
    close(sd);
  }

  return retval;
}

int
lbcd_http_weight(u_int *weight_val, u_int *incr_val,
		 int timeout, char *portarg)
{
  return *weight_val = probe_http("localhost",timeout,portarg);
}

#ifdef MAIN
int
main(int argc, char *argv[])
{
  int status;

  status = probe_http(argv[1],5,NULL);
  printf("http service %savailable\n",status ? "not " : "");
  return status;
}
#endif
