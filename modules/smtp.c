#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

extern int probe_tcp(char *host, char *service, short port,
		     char *replycheck, int timeout);

int
probe_sendmail(char *host, int timeout)
{
  return probe_tcp(host,"smtp",25,"220",timeout);
}

int
lbcd_smtp_weight(u_int *weight_val, u_int *incr_val, int timeout)
{
  return *weight_val = probe_sendmail("localhost",timeout);
}

#ifdef MAIN
int
main(int argc, char *argv[])
{
  int status;

  status = probe_sendmail(argv[1],5);
  printf("sendmail service %savailable\n",status ? "not " : "");
  return status;
}
#endif
