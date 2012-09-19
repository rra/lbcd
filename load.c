/*
 * Default load computation module.
 *
 * This factors in current load, total users, unique users, and how full /tmp
 * is.  It also penalizes systems with /etc/nologin set.  It is suitable for
 * balancing a multiuser compute server.
 *
 * Written by Larry Schwimmer
 * Copyright 1998, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/socket.h>
#include <portable/system.h>

#include <lbcd.h>
#include <lbcdload.h>
#include <util/macros.h>

#ifndef MAX
# define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

/* Multiplier penalty for nearly full tmp directory. */
static int penalty[] = {
    2, 2, 2, 2,                 /* 90-93% */
    4, 4, 4,                    /* 94-96% */
    8, 8,                       /* 97-98% */
    16,                         /* 99% */
    32                          /* 100% */
};

/*
 * Determine the weight for the node and store it in weight_val.  Always use
 * an increment of 100.
 *
 * Since this deals with the raw P_LB_RESPONSE packet, it must convert from
 * network to host order and is thus dependent on the data types in protcol.h.
 * A more elegant approach would be desirable; it might not be so bad just to
 * call the kernel routines twice.
 */
int
lbcd_load_weight(u_int *weight_val, u_int *incr_val, int timeout UNUSED,
                 const char *portarg UNUSED, P_LB_RESPONSE *lb)
{
    int fudge, weight;
    int tmp_used;

    fudge = (ntohs(lb->tot_users) - ntohs(lb->uniq_users)) * 20;
    weight = (ntohs(lb->uniq_users) * 100) + (3 * ntohs(lb->l1)) + fudge;

    /* Heavy penalty for a full /tmp partition. */
    tmp_used = MAX(lb->tmp_full, lb->tmpdir_full);
    if (tmp_used >= 90) {
        if (tmp_used > 100)
            weight = (u_int) -1;
        else
            weight *= penalty[tmp_used - 90];
    }

    /* Do not hand out if /etc/nologin exists. */
    if (access("/etc/nologin", F_OK) == 0)
        weight = (u_int)-1;

    /* Return weight and increment. */
    *weight_val = weight;
    *incr_val = 200;
    return weight;
}
