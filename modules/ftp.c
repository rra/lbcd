#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

extern int probe_tcp(char *host, char *service, short port,
		     char *replycheck, int timeout);

int
probe_ftp(char *host, int timeout)
{
  return probe_tcp(host,"ftp",21,"220",timeout);
}

int
lbcd_ftp_weight(u_int *weight_val, u_int *incr_val, int timeout)
{
  return *weight_val = probe_ftp("localhost",timeout);
}

#ifdef MAIN
int
main(int argc, char *argv[])
{
  int status;

  status = probe_ftp(argv[1],5);
  printf("ftp service %savailable\n",status ? "not " : "");
  return status;
}
#endif
