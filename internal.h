/*
 * Prototypes for internal lbcd functions.
 *
 * Written by Larry Schwimmer
 * Copyright 1996, 1997, 1998, 2004, 2006, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#ifndef LBCD_H
#define LBCD_H 1

#include <config.h>
#include <portable/macros.h>

#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#if HAVE_STDINT_H
# include <stdint.h>
#endif
#if HAVE_SYS_BITYPES_H
# include <sys/bitypes.h>
#endif
#include <sys/types.h>
#include <time.h>               /* time_t */

#include <protocol.h>

#ifndef PID_FILE
# define PID_FILE "/var/run/lbcd.pid"
#endif

/*
 * A weight function takes a pointer to the weight and increment, a timeout, a
 * port argument, and the response argument.
 */
typedef int weight_func_type(uint32_t *, uint32_t *, int, const char *,
                             P_LB_RESPONSE *);

BEGIN_DECLS

/* kernel.c */
extern int kernel_getload(double *l1, double *l5, double *l15);
extern int kernel_getboottime(time_t *boottime);

/* get_user.c */
extern int get_user_stats(int *total, int *unique, int *onconsole,
                          time_t *user_mtime);

/* tmp_free.c */
extern int tmp_full(const char *path);

/* server.c */
extern void lbcd_pack_info(P_LB_RESPONSE *lb, P_HEADER_FULLPTR ph, int simple);
extern void lbcd_test(int argc, char *argv[]);

/* weight.c */
int lbcd_default_weight(P_LB_RESPONSE *lb, uint32_t *weight, uint32_t *incr);
int lbcd_weight_init(const char *cmd, const char *service, int timeout);
void lbcd_setweight(P_LB_RESPONSE *lb, int offset, const char *service);

/* weight.c -- generic routines */
extern weight_func_type lbcd_rr_weight;      /* Round robin */
extern weight_func_type lbcd_cmd_weight;     /* External command */
extern weight_func_type lbcd_unknown_weight; /* Error function */

/* tcp.c -- arbitrary tcp port */
extern weight_func_type lbcd_tcp_weight;

/* load.c -- Default module */
extern weight_func_type lbcd_load_weight;

/* ftp.c */
extern weight_func_type lbcd_ftp_weight;

/* http.c */
extern weight_func_type lbcd_http_weight;

/* imap.c */
extern weight_func_type lbcd_imap_weight;

/* ldap.c */
#ifdef HAVE_LDAP
extern weight_func_type lbcd_ldap_weight;
#endif

/* nntp.c */
extern weight_func_type lbcd_nntp_weight;

/* ntp.c  */
extern weight_func_type lbcd_ntp_weight;

/* pop.c  */
extern weight_func_type lbcd_pop_weight;

/* smtp.c */
extern weight_func_type lbcd_smtp_weight;

/*
 * Extending lbcd locally
 *
 * lbcd contains a decent set of modules and extension mechanisms.  If you
 * wish to extend lbcd, it is pretty simple to write a new module and add it
 * to your local lbcd distribution.  Either as modules/local.c or as something
 * else.  Just include the prototype in lbcdload.h, the .c file in
 * Makefile.am, and the entry in weight.c.
 */
#ifdef HAVE_LOCAL
extern weight_func_type lbcd_local_weight;
#endif

END_DECLS

#endif /* !LBCD_H */
