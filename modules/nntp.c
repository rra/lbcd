#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>

extern int tcp_connect (char *host, char *protocol, int port);
extern int lbcd_check_reply(int sd, int timeout, char *token);

int
probe_nntp(char *host)
{
  int sd;
  int retval = 0;

  if ((sd = tcp_connect(host ? host : "localhost","nntp",119)) == -1) {
    return -1;
  } else {
    retval = lbcd_check_reply(sd,5,"200");
    write(sd,"quit\r\n",6);
    close(sd);
  }
  return retval;
}

#ifdef MAIN
int
main(int argc, char *argv[])
{
  int status;

  status = probe_nntp(argv[1]);
  printf("nntp service %savailable\n",status ? "not " : "");
  return status;
}
#endif
