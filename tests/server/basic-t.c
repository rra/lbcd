/*
 * Test for basic lbcd server functionality.
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


/*
 * Check a reply packet given a request packet.  We don't check the actual
 * values of most of the parameters other than insuring that they're basically
 * sane (load below 200, for instance).  Takes the request, the reply, the
 * reply size, and a short string to use to name this test.
 */
static void
is_sane_reply(struct lbcd_request *request, struct lbcd_reply *reply,
              size_t size, const char *name)
{
    int services, protocol;
    size_t expected;
    time_t now;

    /* Determine the expected number of services and size of response. */
    protocol = ntohs(request->h.version);
    services = (protocol == 3) ? ntohs(request->h.status) : 0;
    expected = sizeof(*reply);
    expected -= sizeof(reply->weights[0]) * (LBCD_MAX_SERVICES - services);

    /* Check the size of the reply. */
    is_int(expected, size, "Reply to %s is correct size", name);

    /* Check the reply header. */
    is_int(ntohs(request->h.version), ntohs(reply->h.version),
           "...and has correct version");
    is_int(ntohs(request->h.id), ntohs(reply->h.id),
           "...and has correct id");
    is_int(LBCD_OP_LBINFO, ntohs(reply->h.op),
           "...and has correct operation");
    is_int(LBCD_STATUS_OK, ntohs(reply->h.status),
           "...and has correct status");

    /* Check the general system information. */
    now = time(NULL);
    ok(ntohl(reply->boot_time) > 946713600,
       "...boot time later than 2000-01-01");
    ok(ntohl(reply->boot_time) < now, "...boot time before the current time");
    ok(labs((time_t) ntohl(reply->current_time) - now) < 2,
       "...current time within two seconds");
    ok(ntohl(reply->user_mtime) > 946713600,
       "...user mtime later than 2000-01-01");
    ok(ntohl(reply->user_mtime) < now,
       "...user mtime before the current time");
    ok(ntohs(reply->l1) < 20000, "...1 minute load less than 200");
    ok(ntohs(reply->l5) < 20000, "...1 minute load less than 200");
    ok(ntohs(reply->l15) < 20000, "...1 minute load less than 200");
    ok(ntohs(reply->uniq_users) <= ntohs(reply->tot_users),
       "...unique users <= total users");
    ok(reply->on_console == 0 || reply->on_console == 1,
       "...on_console is either 0 or 1");
    is_int(0, reply->reserved, "...reserved field is zero");
    ok(reply->tmp_full <= 100, "...tmp_full <= 100");
    ok(reply->tmpdir_full <= 100, "...tmpdir_full <= 100");
    is_int(0, reply->pad, "...padding field is zero");
    is_int(services, reply->services, "...correct number of extra services");

    /* For protocol three replies, check the first service. */
    if (protocol == 3)
        is_int(200, ntohl(reply->weights[0].host_incr),
               "...default service increment is correct");
}


int
main(void)
{
    socket_type fd;
    struct sockaddr_in sin;
    size_t size;
    ssize_t result;
    struct lbcd_reply reply;
    struct lbcd_request request;

    /* Declare a plan. */
    plan(68);

    /* Start the lbcd daemon, allowing load and rr services. */
    lbcd_start("-a", "load", "-a", "rr", NULL);

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

    /* Send a simple version three query and get the reply. */
    memset(&request, 0, sizeof(request));
    request.h.version = htons(3);
    request.h.id = htons(10);
    request.h.op = htons(LBCD_OP_LBINFO);
    result = send(fd, &request, sizeof(struct lbcd_header), 0);
    if (result != (ssize_t) sizeof(struct lbcd_header))
        sysbail("cannot send simple version 3 query");
    memset(&reply, 0, sizeof(reply));
    result = recv(fd, &reply, sizeof(reply), 0);

    /* Check that the reply is what we expect. */
    is_sane_reply(&request, &reply, result, "simple protocol three query");

    /* Send a version two query and get the reply. */
    request.h.version = htons(2);
    request.h.id = htons(20);
    request.h.status = htons(200);
    result = send(fd, &request, sizeof(struct lbcd_header), 0);
    if (result != (ssize_t) sizeof(struct lbcd_header))
        sysbail("cannot send version 2 query");
    memset(&reply, 0, sizeof(reply));
    result = recv(fd, &reply, sizeof(reply), 0);

    /* Check that the reply is what we expect. */
    is_sane_reply(&request, &reply, result, "protocol two query");

    /*
     * Send a more complex version three query with three requested services:
     * default, which should always be allowed, load, which should match
     * default, and rr.
     */
    request.h.version = htons(3);
    request.h.status = htons(3);
    strlcpy(request.names[0], "default", sizeof(request.names[0]));
    strlcpy(request.names[1], "load", sizeof(request.names[0]));
    strlcpy(request.names[2], "rr", sizeof(request.names[0]));
    size = sizeof(struct lbcd_header) + 3 * sizeof(lbcd_name_type);
    result = send(fd, &request, size, 0);
    if (result != (ssize_t) size)
        sysbail("cannot send complex version 3 query");
    memset(&reply, 0, sizeof(reply));
    result = recv(fd, &reply, sizeof(reply), 0);

    /* Check that the reply is what we expect. */
    is_sane_reply(&request, &reply, result, "complex protocol three query");
    is_int(ntohl(reply.weights[0].host_weight),
           ntohl(reply.weights[1].host_weight),
           "...default service weight matches first weight");
    is_int(ntohl(reply.weights[0].host_incr),
           ntohl(reply.weights[1].host_incr),
           "...default service increment matches first increment");
    is_int(ntohl(reply.weights[0].host_weight),
           ntohl(reply.weights[2].host_weight),
           "...load service weight matches first weight");
    is_int(ntohl(reply.weights[0].host_incr),
           ntohl(reply.weights[2].host_incr),
           "...load service increment matches first increment");
    is_int(1, ntohl(reply.weights[3].host_weight),
           "...rr service weight is 1");
    is_int(1, ntohl(reply.weights[3].host_incr),
           "...rr service increment is 1");

    /* All done.  Clean up and return. */
    close(fd);
    return 0;
}
