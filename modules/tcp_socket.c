/*
 * Generic TCP connection code.
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
#include <util/network.h>
#include <util/xmalloc.h>


/*
 * Connect to a host with specified protocol using TCP.  Takes the name of the
 * server to connect to, the name of the protocol, and the port to use if the
 * protocol name cannot be resolved to a port.  If the port is 0, the check
 * will fail if the protocol name cannot be resolved to a port.
 *
 * Returns the file descriptor of the connected socket on success and
 * INVALID_SOCKET on failure.
 */
socket_type
tcp_connect(const char *host, const char *protocol, int port)
{
    struct addrinfo *ai, hints;
    char *p;
    int status;
    socket_type fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(host, protocol, &hints, &ai);
    if (status != 0 && port != 0) {
        xasprintf(&p, "%d", port);
        hints.ai_flags = AI_NUMERICSERV;
        status = getaddrinfo(host, p, &hints, &ai);
        free(p);
    }
    if (status != 0)
        return -1;
    fd = network_connect(ai, NULL, 0);
    freeaddrinfo(ai);
    return fd;
}
