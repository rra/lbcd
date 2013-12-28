/*
 * Test for lbcd server error handling and replies.
 *
 * Written by Russ Allbery <eagle@eyrie.org>
 * Copyright 2013
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/socket.h>
#include <portable/system.h>

#include <time.h>

#include <server/protocol.h>
#include <tests/tap/basic.h>
#include <tests/tap/lbcd.h>
#include <util/network.h>


int
main(void)
{
    socket_type fd;
    struct sockaddr_in sin;
    size_t size;
    ssize_t result;
    struct lbcd_reply reply;
    struct lbcd_request request;
    char *buffer;

    /* Declare a plan. */
    plan(15);

    /* Start the lbcd daemon with no special flags. */
    lbcd_start(NULL);

    /* Set up our client socket. */
    fd = network_client_create(PF_INET, SOCK_DGRAM, "127.0.0.1");
    if (fd == INVALID_SOCKET)
        sysbail("cannot create client socket");
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(14330);
    sin.sin_addr.s_addr = htonl(0x7f000001UL);
    if (connect(fd, (struct sockaddr *) &sin, sizeof(sin)) < 0)
        sysbail("cannot connect client socket");

    /* Send a truncated packet.  There should be no reply to this. */
    memset(&request, 0, sizeof(request));
    request.h.version = htons(3);
    request.h.id = htons(10);
    request.h.op = htons(LBCD_OP_LBINFO);
    result = send(fd, &request, sizeof(struct lbcd_header) / 2, 0);
    if (result != (ssize_t) sizeof(struct lbcd_header) / 2)
        sysbail("cannot send truncated request");

    /* Send a request with too many services.  Similarly, no reply. */
    memset(&request, 0, sizeof(request));
    request.h.version = htons(3);
    request.h.id = htons(10);
    request.h.op = htons(LBCD_OP_LBINFO);
    request.h.status = htons(LBCD_MAX_SERVICES + 1);
    size = sizeof(struct lbcd_request) + sizeof(lbcd_name_type);
    buffer = bcalloc(1, size);
    memcpy(buffer, &request, sizeof(request));
    result = send(fd, buffer, size, 0);
    if (result != (ssize_t) size)
        sysbail("cannot send request with too many services");
    free(buffer);

    /* Send a too-long request.  Similarly, no reply. */
    request.h.status = 0;
    result = send(fd, &request, sizeof(request), 0);
    if (result != (ssize_t) sizeof(request))
        sysbail("cannot send too-long request");

    /*
     * Okay, finally a request to which we should get a reply.  Send a request
     * for an unknown protocol version.
     */
    request.h.version = htons(LBCD_VERSION + 1);
    result = send(fd, &request, sizeof(struct lbcd_header), 0);
    if (result != (ssize_t) sizeof(struct lbcd_header))
        sysbail("cannot send unknown version query");
    memset(&reply, 0, sizeof(reply));
    result = recv(fd, &reply, sizeof(reply), 0);

    /*
     * We should get back an error message.  This is where we also test that
     * we're in synchronization and aren't getting weird errors from our
     * previous bad packets.
     */
    is_int(sizeof(struct lbcd_header), result,
           "Unknown version error reply is correct length");
    is_int(3, ntohs(reply.h.version), "...and is protocol version three");
    is_int(10, ntohs(reply.h.id), "...with correct id");
    is_int(LBCD_OP_LBINFO, ntohs(reply.h.op), "...and correct op");
    is_int(LBCD_STATUS_VERSION, ntohs(reply.h.status),
           "...and correct error status");

    /* Send a request for a service we're not allowed to query. */
    request.h.version = htons(3);
    request.h.status = htons(1);
    strlcpy(request.names[0],"rr", sizeof(request.names[0]));
    size = sizeof(struct lbcd_header) + sizeof(lbcd_name_type);
    result = send(fd, &request, size, 0);
    if (result != (ssize_t) size)
        sysbail("cannot send bad service query");
    memset(&reply, 0, sizeof(reply));
    result = recv(fd, &reply, sizeof(reply), 0);

    /* Check the resulting error message. */
    is_int(sizeof(struct lbcd_header), result,
           "Bad service error reply is correct length");
    is_int(3, ntohs(reply.h.version), "...and is protocol version three");
    is_int(10, ntohs(reply.h.id), "...with correct id");
    is_int(LBCD_OP_LBINFO, ntohs(reply.h.op), "...and correct op");
    is_int(LBCD_STATUS_ERROR, ntohs(reply.h.status),
           "...and correct error status");

    /* Send a request for an unknown op code. */
    request.h.op = htons(10);
    request.h.status = 0;
    result = send(fd, &request, sizeof(struct lbcd_header), 0);
    if (result != (ssize_t) sizeof(struct lbcd_header))
        sysbail("cannot send unknown op query");
    memset(&reply, 0, sizeof(reply));
    result = recv(fd, &reply, sizeof(reply), 0);

    /* Check the resulting error message. */
    is_int(sizeof(struct lbcd_header), result,
           "Unknown op error reply is correct length");
    is_int(3, ntohs(reply.h.version), "...and is protocol version three");
    is_int(10, ntohs(reply.h.id), "...with correct id");
    is_int(10, ntohs(reply.h.op), "...and correct op");
    is_int(LBCD_STATUS_UNKNOWN_OP, ntohs(reply.h.status),
           "...and correct error status");

    /* All done.  Clean up and return. */
    close(fd);
    return 0;
}
