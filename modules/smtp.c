#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern int tcp_connect (char *host, char *protocol, int port);
extern int lbcd_check_reply(int sd, int timeout, char *token);

int
probe_sendmail(char *host)
{
  int sd;
  int retval = 0;

  if ((sd = tcp_connect(host ? host : "localhost","smtp",25)) == -1) {
    return -1;
  } else {
    retval = lbcd_check_reply(sd,5,"220");
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

  status = probe_sendmail(argv[1]);
  printf("sendmail service %savailable\n",status ? "not " : "");
  return status;
}
#endif
