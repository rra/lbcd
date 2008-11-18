#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "lbcd.h"
#include "lbcdload.h"
#include "modules/modules.h"

static int
probe_ftp(const char *host, int timeout)
{
  return probe_tcp(host,"ftp",21,"220",timeout);
}

int
lbcd_ftp_weight(u_int *weight_val, u_int *incr_val UNUSED, int timeout,
                const char *portarg UNUSED, P_LB_RESPONSE *lb UNUSED)
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
