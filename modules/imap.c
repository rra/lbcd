#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

extern int tcp_connect (char *host, char *protocol, int port);
extern int lbcd_check_reply(int sd, int timeout, char *token);

int
probe_imap(char *host, int timeout)
{
  int sd;
  int retval = 0;

  if ((sd = tcp_connect(host ? host : "localhost","imap",143)) == -1) {
    return -1;
  } else {
    retval = lbcd_check_reply(sd,timeout,"* OK");
    write(sd,"tag logout\r\n",12);
    close(sd);
  }
  return retval;
}

int
lbcd_imap_weight(u_int *weight_val, u_int *incr_val, int timeout)
{
  return *weight_val = probe_imap("localhost",timeout);
}

#ifdef MAIN
int
main(int argc, char *argv[])
{
  int status;

  status = probe_imap(argv[1]);
  printf("imap service %savailable\n",status ? "not " : "");
  return status;
}
#endif
