/*
 * Generic UDP connection code.
 *
 * Written by Larry Schwimmer
 * Copyright 1997, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/socket.h>
#include <portable/system.h>

#include <ctype.h>
#include <errno.h>

#include <modules/modules.h>

/*
 * Connect to a host with specified protocol using UDP.
 *
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
udp_connect(const char *host, const char *protocol, int port)
{
    struct servent *se;
    unsigned int addr;
    struct hostent *he;
    struct sockaddr_in serv_addr;
    int sd;

    /* Assign port. */
    memset(&serv_addr, 0, sizeof(serv_addr));
    if ((protocol != NULL
         && (se = getservbyname (protocol, "udp")) != NULL)
        || (port != 0 && (se = getservbyport(htons(port), "udp")) != NULL))
        serv_addr.sin_port = se->s_port;
    else if (port != 0)
        serv_addr.sin_port = htons(port);
    endservent();

    /* First check if valid IP address.  Otherwise check if valid name. */
    addr = inet_addr(host);
    if (addr != (in_addr_t) -1) {
        he = gethostbyaddr(&addr, sizeof(unsigned int), AF_INET);
        if (he == NULL)
            return -1;
    } else {
        he = gethostbyname(host);
        if (he == NULL)
            return -1;
    }

    /* Set up socket connection. */
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr, he->h_addr, sizeof(he->h_addr));
    sd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
        return -1;
    if (connect(sd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        close(sd);
        return -1;
    }
    return sd;
}
