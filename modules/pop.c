#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

extern int tcp_connect (char *host, char *protocol, int port);
extern int lbcd_check_reply(int sd, int timeout, char *token);

int
probe_pop(char *host)
{
  int sd;
  int retval = 0;

  if ((sd = tcp_connect(host ? host : "localhost","pop",110)) == -1) {
    return -1;
  } else {
    retval = lbcd_check_reply(sd,5,"+OK");
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

  status = probe_pop(argv[1]);
  printf("pop service %savailable\n",status ? "not " : "");
  return status;
}
#endif
