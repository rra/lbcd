#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "lbcdload.h"
#include "modules/modules.h"

static int
probe_sendmail(const char *host, int timeout)
{
  return probe_tcp(host,"smtp",25,"220",timeout);
}

int
lbcd_smtp_weight(u_int *weight_val, u_int *incr_val UNUSED, int timeout,
                 const char *portarg UNUSED, P_LB_RESPONSE *lb UNUSED)
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
