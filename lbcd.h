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

#include <portable/macros.h>
#include <time.h>

#include <protocol.h>

/* Used for unused parameters to silence gcc warnings. */
#define UNUSED __attribute__((__unused__))

/*
 * Defines
 */
#ifndef PROGNAME
# define PROGNAME "lbcd"
#endif

#ifndef PID_FILE
# define PID_FILE "/var/run/lbcd.pid"
#endif

/*
 * Prototypes
 */

BEGIN_DECLS

/* kernel.c */
extern int kernel_getload(double *l1, double *l5, double *l15);
extern int kernel_getboottime(time_t *boottime);

/* get_user.c */
extern int get_user_stats(int *total, int *unique,
              int *onconsole, time_t *user_mtime);

/* tmp_free.c */
extern int tmp_full(const char *path);

/* server.c */
extern void lbcd_pack_info(P_LB_RESPONSE *lb, P_HEADER_FULLPTR ph, int simple);
extern void lbcd_test(int argc, char *argv[]);

/* util.c */
extern void util_log_info(const char *fmt, ...);
extern void util_log_error(const char *fmt, ...);
extern pid_t util_get_pid_from_file(const char *file);
extern int util_write_pid_in_file(const char *file);
extern void util_log_close(void);

/* weight.c */
int lbcd_default_weight(P_LB_RESPONSE *lb, u_int *weight, u_int *incr);
int lbcd_weight_init(const char *cmd, const char *service, int timeout);
void lbcd_setweight(P_LB_RESPONSE *lb, int offset, const char *service);

END_DECLS

#endif /* !LBCD_H */
