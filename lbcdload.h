#ifndef __LBCDLOAD_H__
#define __LBCDLOAD_H__

#include "lbcd.h"

/*
 * A weight function takes a pointer to the weight and increment
 * and possibly an extra argument and sets the values.
 * Since C does not support closures or polymorphic functions,
 * the arguments to the function are not listed in order to
 * avoid casts.  The three real possible prototypes include:
 *
 * NONE: int weight_func_t(u_int *,u_int *,P_LB_RESPONSE *);
 *   LB: int weight_func_t(u_int *,u_int *,P_LB_RESPONSE *);
 * PORT: int weight_func_t(u_int *,u_int *,char *);
 */
typedef int weight_func_t();

/* lbcdcmd_t: third argument of weight function
 */
typedef enum {
  LBCD_ARGNONE    = 0,
  LBCD_ARGLB      = 1,
  LBCD_ARGPORT    = 2
} lbcdcmd_t;

typedef struct lbcd_func_tab {
  LBCD_SERVICE_REQ service;
  weight_func_t *function;
  lbcdcmd_t argument;
} lbcd_func_tab_t;

/*
 * Prototypes
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Load balancing modules
 */

/* weight.c -- generic routines */
extern weight_func_t lbcd_rr_weight;	/* round robin */
extern weight_func_t lbcd_cmd_weight;	/* external command */
extern weight_func_t lbcd_unknown_weight; /* error function */

/* tcp.c -- arbitrary tcp port */
extern weight_func_t lbcd_tcp_weight;

/* load.c -- Default module */
extern weight_func_t lbcd_load_weight;

/* ftp.c */
extern weight_func_t lbcd_ftp_weight;

/* http.c */
extern weight_func_t lbcd_http_weight;

/* imap.c */
extern weight_func_t lbcd_imap_weight;

/* ldap.c */
#ifdef HAVE_LDAP
extern weight_func_t lbcd_ldap_weight;
#endif

/* nntp.c */
extern weight_func_t lbcd_nntp_weight;

/* ntp.c  */
extern weight_func_t lbcd_ntp_weight;

/* pop.c  */
extern weight_func_t lbcd_pop_weight;

/* smtp.c */
extern weight_func_t lbcd_smtp_weight;

/*
 * Extending lbcd locally
 *
 * lbcd contains a decent set of modules and extension mechanisms.
 * If you wish to extend lbcd, it is pretty simple to write a new
 * module and add it to your local lbcd distribution.  Either as
 * modules/local.c or as something else.  Just include the prototype
 * in lbcdload.h, the .c file in Makefile.in, and the entry in
 * weight.c.
 */
/* local.c */
#ifdef HAVE_LOCAL
extern weight_func_t lbcd_local_weight;
#endif

#ifdef __cplusplus
}
#endif

#endif /* __LBCDLOAD_H__ */