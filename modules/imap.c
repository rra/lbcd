#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

extern int tcp_connect (char *host, char *protocol, int port);
extern int lbcd_check_reply(int sd, int timeout, char *token);

int
probe_imap(char *host)
{
  int sd;
  int retval = 0;

  if ((sd = tcp_connect(host ? host : "localhost","imap",143)) == -1) {
    return -1;
  } else {
    retval = lbcd_check_reply(sd,5,"* OK");
    write(sd,"tag logout\r\n",12);
    close(sd);
  }
  return retval;
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
