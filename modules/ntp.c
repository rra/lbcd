#include <stdio.h>

extern int udp_connect (char *host, char *protocol, int port);
extern int monlist(int sd);

int
probe_ntp(char *host)
{
  int sd;
  int retval = 0;

  if ((sd = udp_connect(host ? host : "localhost","ntp",123)) == -1) {
    return -1;
  } else {
    retval = monlist(sd);
    close(sd);
  }

  return retval;
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
