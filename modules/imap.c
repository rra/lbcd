#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "lbcdload.h"
#include "modules/modules.h"

static int
probe_imap(const char *host, int timeout)
{
  int sd;
  int retval = 0;

  if ((sd = tcp_connect(host ? host : "localhost","imap",143)) == -1) {
    return -1;
  } else {
    retval = lbcd_check_reply(sd,timeout,"* OK");
    write(sd,"tag logout\r\n",12);
    close(sd);
  }
  return retval;
}

int
lbcd_imap_weight(u_int *weight_val, u_int *incr_val UNUSED, int timeout,
                 const char *portarg UNUSED, P_LB_RESPONSE *lb UNUSED)
{
  return *weight_val = probe_imap("localhost",timeout);
}

#ifdef MAIN
int
main(int argc, char *argv[])
{
  int status;

  status = probe_imap(argv[1]);
  printf("imap service %savailable\n",status ? "not " : "");
  return status;
}
#endif
