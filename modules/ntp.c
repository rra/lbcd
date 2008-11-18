#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "lbcdload.h"
#include "modules/modules.h"
#include "modules/monlist.h"

static int
probe_ntp(const char *host, int timeout)
{
  int sd;
  int retval = 0;

  if ((sd = udp_connect(host ? host : "localhost","ntp",123)) == -1) {
    return -1;
  } else {
    retval = monlist(sd,timeout);
    close(sd);
  }

  return retval;
}

int
lbcd_ntp_weight(u_int *weight_val, u_int *incr_val UNUSED, int timeout,
                const char *portarg UNUSED, P_LB_RESPONSE *lb UNUSED)
{
  *weight_val = (u_int)probe_ntp("localhost",timeout);
  return (*weight_val == (u_int) -1) ? -1 : 0;
}

#ifdef MAIN
int
main(int argc, char *argv[])
{
  int status;

  status = probe_ntp(argv[1]);
  if (status <= 0) {
    printf("ntp service not available\n");
    return -1;
  } else {
    printf("%s ntp service has %d peers\n",
	   argv[1] ? argv[1] : "localhost",
	   status);
  }
  return 0;
}
#endif
