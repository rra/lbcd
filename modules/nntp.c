#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>

extern int probe_tcp(char *host, char *service, short port,
		     char *replycheck, int timeout);

int
probe_nntp(char *host, int timeout)
{
  return probe_tcp(host,"nntp",119,"200",timeout);
}

int
lbcd_nntp_weight(u_int *weight_val, u_int *incr_val, int timeout)
{
  return *weight_val = probe_nntp("localhost",timeout);
}

#ifdef MAIN
int
main(int argc, char *argv[])
{
  int status;

  status = probe_nntp(argv[1],5);
  printf("nntp service %savailable\n",status ? "not " : "");
  return status;
}
#endif
