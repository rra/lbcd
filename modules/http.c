#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>

extern int tcp_connect (char *host, char *protocol, int port);

#define QUERY "GET / HTTP/1.0\r\n\r\n"

int
probe_http(char *host)
{
  int sd;
  int retval = 0;

  if ((sd = tcp_connect(host ? host : "localhost","http",80)) == -1) {
    return -1;
  } else {
    struct timeval tv = { 1, 0 };
    fd_set rset;
    char buf[17];
    int i;

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

#ifdef MAIN
int
main(int argc, char *argv[])
{
  int status;

  status = probe_http(argv[1]);
  printf("http service %savailable\n",status ? "not " : "");
  return status;
}
#endif
