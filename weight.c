#include "lbcd.h"

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

void
lbcd_default_weight(P_LB_RESPONSE *lb, int *weight_val, int *incr_val)
{
  int fudge, weight;
  int tmp_used;

  fudge = (lb->tot_users - lb->uniq_users) * 20;
  weight = (lb->uniq_users * 100) + (3 * lb->l1) + fudge;

  /* heavy penalty for a full /tmp partition */
  tmp_used = MAX(lb->tmp_full,lb->tmpdir_full);
  switch(tmp_used) {
  default:			/* over 100% */
    weight = (u_short)-1;
    break;
  case 100:
    weight *= 2;
  case 99:
    weight *= 2;
  case 98:
  case 97:
    weight *= 2;
  case 96:
  case 95:
  case 94:
    weight *= 2;
  case 93:
  case 92:
  case 91:
  case 90:
    weight = weight * 2;
  }
  if (weight > (u_short)-1)
    weight = (u_short)-1;		/* maximum */
    
  *weight_val = weight;
  *incr_val = 200;
}
