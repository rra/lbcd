#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>

extern int tcp_connect (char *host, char *protocol, int port);

int
probe_sendmail(char *host)
{
  int sd;
  int retval = 0;

  if ((sd = tcp_connect(host ? host : "localhost","smtp",25)) == -1) {
    return -1;
  } else {
    struct timeval tv = { 1, 0 };
    fd_set rset;
    char buf[4];
    int i;

    FD_ZERO(&rset);
    FD_SET(sd,&rset);
    if (select(sd+1, &rset, NULL, NULL, &tv) > 0) {
      buf[sizeof(buf)-1] = '\0';
      if (read(sd,buf,sizeof(buf)-1) > 0) {
	if (strcmp(buf,"220") != 0) {
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

  status = probe_sendmail(argv[1]);
  printf("sendmail service %savailable\n",status ? "not " : "");
  return status;
}
#endif
