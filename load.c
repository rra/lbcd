/*
 * modules/load.c
 *
 * Default load computation module, suitable for balancing a
 * multiuser compute server.
 */

#include "lbcd.h"

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

/* Multiplier penalty for nearly full tmp directory */
static int penalty[] = {
  2,2,2,2,			/* 90-93% */
  4,4,4,			/* 94-96% */
  8,8,				/* 97-98% */
  16,				/* 99% */
  32				/* 100% */
};

void
lbcd_load_weight(P_LB_RESPONSE *lb, int *weight_val, int *incr_val)
{
  int fudge, weight;
  int tmp_used;

  fudge = (lb->tot_users - lb->uniq_users) * 20;
  weight = (lb->uniq_users * 100) + (3 * lb->l1) + fudge;

  /* Heavy penalty for a full /tmp partition */
  tmp_used = MAX(lb->tmp_full,lb->tmpdir_full);
  if (tmp_used >= 90) {
    if (tmp_used > 100)
      weight = (u_int)-1;
    else
      weight *= penalty[tmp_used-90];
  }

  /* Do not hand out if /etc/nologin exists */
  if (access("/etc/nologin",F_OK) == 0) {
    weight = (u_int)-1;
  }

  *weight_val = weight;
  *incr_val = 200;
}
