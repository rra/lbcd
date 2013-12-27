/*
 * lbcd client-server routines.
 *
 * Obtains and sends polling information.  Also acts as a test driver.
 *
 * Written by Larry Schwimmer
 * Copyright 1996, 1997, 1998, 2004, 2006, 2012, 2013
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/socket.h>
#include <portable/system.h>

#include <time.h>

#include <server/internal.h>
#include <util/vector.h>


/*
 * Set the waits and increments in the response.
 */
static void
lbcd_set_load(struct lbcd_reply *lb, struct vector *services)
{
    int i, numserv;

    /* Clear pad and set number of requested services */
    lb->pad = 0;
    lb->services = numserv = services->count;

    /* Set default weight. */
    lbcd_setweight(lb, 0, "default");

    /* If the sentinel file exists, override the weight and set it to max. */
    if (access(LBCD_SENTINEL_FILE, F_OK) == 0)
        lb->weights[0].host_weight = (uint32_t) -1;

    /* Convert to network byte order. */
    lb->weights[0].host_weight = htonl(lb->weights[0].host_weight);
    lb->weights[0].host_incr = htonl(lb->weights[0].host_incr);

    /* Set requested services, if any */
    for (i = 1; i <= numserv; i++) {
        lbcd_setweight(lb, i, services->strings[i - 1]);
        lb->weights[i].host_weight = htonl(lb->weights[i].host_weight);
        lb->weights[i].host_incr = htonl(lb->weights[i].host_incr);
    }
}


/*
 * Convert a protocol 3 packet to a form a protocol 2 server will use
 * properly.
 *
 * Version 2 of the protocol had no mechanism for client-computed load.  To
 * work around this problem, we just lie to the version 2 server such that the
 * weight lbnamed will compute will be the weight we desire.  Note that we
 * have no control over the increment for version 2 lbnamed -- that value is
 * hard-coded.
 */
static void
lbcd_proto2_convert(struct lbcd_reply *lb)
{
    uint32_t weightval_i;
    uint16_t weightval_s;

    /* Convert host weight to a short, handling network byte order */
    weightval_i = ntohl(lb->weights[0].host_weight);
    if (weightval_i > (uint16_t) -1)
        weightval_s = (uint16_t) -1;
    else
        weightval_s = weightval_i;

    /*
     * lbnamed v2 only used l1, tot_users, and uniq_users, although Rob thinks
     * all the loads may be used, so set them all just in case.
     */
    lb->l1 = htons(weightval_s);
    lb->l5 = htons(weightval_s);
    lb->l15 = htons(weightval_s);
    lb->tot_users = 0;
    lb->uniq_users = 0;
}


/*
 * Obtain all of our response information and store it in the response struct.
 */
void
lbcd_pack_info(struct lbcd_reply *lb, unsigned int protocol,
               struct vector *services, int simple)
{
    double l1, l5, l15;
    time_t bt, ct;
    int tu, uu, oc;
    time_t umtime;

    /* Timestamps. */
    kernel_getboottime(&bt);
    lb->boot_time = htonl(bt);
    time(&ct);
    lb->current_time = htonl(ct);

    /* Load. */
    kernel_getload(&l1,&l5,&l15);
    lb->l1 = htons((uint16_t) (l1 * 100));
    lb->l5 = htons((uint16_t) (l5 * 100));
    lb->l15 = htons((uint16_t) (l15 * 100));

    /* Users. */
    get_user_stats(&tu,&uu,&oc,&umtime);
    lb->tot_users = htons(tu);
    lb->uniq_users = htons(uu);
    lb->on_console = oc;
    lb->user_mtime = htonl(umtime);

    /* Additional fields. */
    lb->reserved = 0;
    lb->tmp_full = tmp_full("/tmp");
#ifdef P_tmpdir
    lb->tmpdir_full = tmp_full(P_tmpdir);
#else
    lb->tmpdir_full = lb->tmp_full;
#endif

    /* Weights and increments. */
    lbcd_set_load(lb, services);

    /* Backward compatibility. */
    if (!simple && protocol < 3)
        lbcd_proto2_convert(lb);
}


/*
 * Test lbcd by looking for the same data that we would return over the
 * network but print it to standard output instead.  Takes the non-option
 * arguments, which are taken to be a list of services whose weight modules
 * should be run.  The special module "v2" means to do protocol version 2
 * instead of 3 (the default).
 */
void
lbcd_test(int argc, char *argv[])
{
    struct lbcd_reply lb;
    struct lbcd_request ph;
    struct vector *services;
    int i;

    /* Create query packet. */
    lb.h.version = ph.h.version = LBCD_VERSION;
    lb.h.id      = ph.h.id = 0;
    lb.h.op      = ph.h.op = LBCD_OP_LBINFO;
    lb.h.status  = ph.h.status = 0;

    /* Handle version 2 option. */
    if (argv[0] && strcmp(argv[0],"v2") == 0) {
        argc--;
        argv++;
        lb.h.version = ph.h.version = 2;
    }

    /* Fill in service requests. */
    services = vector_new();
    if (argc > 0 && lb.h.version == 3) {
        ph.h.status = argc > LBCD_MAX_SERVICES ? LBCD_MAX_SERVICES : argc;
        for (i = 0; i < argc; i++) {
            if (i >= LBCD_MAX_SERVICES)
                break;
            if (argv[i] == NULL)
                break;
            vector_add(services, argv[i]);
        }
    }

    /* Fill reply. */
    lbcd_pack_info(&lb, lb.h.version, services, 0);

    /* Print results. */
    printf("PROTOCOL %u\n", (unsigned int) lb.h.version);
    printf("\n");
    printf("MACHINE STATUS:\n");
    printf("l1           = %u\n",  (unsigned int) ntohs(lb.l1));
    printf("l5           = %u\n",  (unsigned int) ntohs(lb.l5));
    printf("l15          = %u\n",  (unsigned int) ntohs(lb.l15));
    printf("current_time = %lu\n", (unsigned long) ntohl(lb.current_time));
    printf("boot_time    = %lu\n", (unsigned long) ntohl(lb.boot_time));
    printf("user_mtime   = %lu\n", (unsigned long) ntohl(lb.user_mtime));
    printf("tot_users    = %u\n",  (unsigned int) ntohs(lb.tot_users));
    printf("uniq_users   = %u\n",  (unsigned int) ntohs(lb.uniq_users));
    printf("on_console   = %u\n",  (unsigned int) lb.on_console);
    printf("tmp_full     = %u\n",  (unsigned int) lb.tmp_full);
    printf("tmpdir_full  = %u\n",  (unsigned int) lb.tmpdir_full);
    printf("\n");
    printf("SERVICES: %u\n", (unsigned int) lb.services);
    for (i = 0; i <= lb.services; i++)
        printf("%d: weight %10lu increment %10lu name %s\n", i,
               (unsigned long) ntohl(lb.weights[i].host_weight),
               (unsigned long) ntohl(lb.weights[i].host_incr),
               i ? ph.names[i - 1] : "default");
    vector_free(services);
    exit(0);
}
