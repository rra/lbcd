/*
 * socket.c
 * Larry Schwimmer (schwim@cs.stanford.edu)
 *
 * Generic tcp routines
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/* tcp_connect
 *
 * Connect to a host with specified protocol using tcp.
 * Input:
 *	host		name of server to connect to
 *	protocol	protocol to use
 *	[port]		port to use if protocol is unknown (use 0 if
 *			default port is not wanted)
 * Returns:
 *	file descriptor number of the socket connection on success
 *	-1	on failure
 */
int
tcp_connect (char *host, char *protocol, int port)
{
  struct servent *se;
  unsigned int addr;
  struct hostent *he;
  struct sockaddr_in serv_addr;
  int sd;
  extern int errno;

  /* Assign port */
  memset ((char *) &serv_addr, 0, sizeof (serv_addr));
  if ((se = getservbyname (protocol, "tcp")) != NULL ||
      (port && (se = getservbyport(htons(port),"tcp")) != NULL)) {
    serv_addr.sin_port = se->s_port;
  } else if (port) {
    serv_addr.sin_port = htons(port);
  }
  endservent();

  /* First check if valid IP address.  Otherwise check if valid name. */
  if ((addr = inet_addr(host)) != -1) {
    if ((he = gethostbyaddr ((char *)&addr, sizeof(unsigned int),
			     AF_INET)) == NULL) {
      return -1;
    }
  } else if ((he = gethostbyname (host)) == NULL) {
    return -1;
  }

  /* Set up socket connection */
  serv_addr.sin_family = AF_INET;
  memcpy (&serv_addr.sin_addr, he->h_addr, he->h_length);

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    return -1;
  }

  if (connect (sd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0 ) {
    (void)close(sd); /* Back out */
    return -1;
  }

  return sd;
}
