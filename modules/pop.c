#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "lbcdload.h"
#include "modules/modules.h"

static int
probe_pop(const char *host, int timeout)
{
  return probe_tcp(host,"pop",110,"+OK",timeout);
}

int
lbcd_pop_weight(u_int *weight_val, u_int *incr_val UNUSED, int timeout,
                const char *portarg UNUSED, P_LB_RESPONSE *lb UNUSED)
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
