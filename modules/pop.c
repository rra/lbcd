#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

extern int probe_tcp(char *host, char *service, short port,
		     char *replycheck, int timeout);

int
probe_pop(char *host, int timeout)
{
  return probe_tcp(host,"pop",110,"+OK",timeout);
}

int
lbcd_pop_weight(u_int *weight_val, u_int *incr_val, int timeout)
{
  return *weight_val = probe_pop("localhost",timeout);
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
