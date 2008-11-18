#ifndef __LBCD_H__
#define __LBCD_H__

/*
 * __attribute__ is available in gcc 2.5 and later, but only with gcc 2.7
 * could you use the __format__ form of the attributes, which is what we use
 * (to avoid confusion with other macros).
 */
#ifndef __attribute__
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __attribute__(spec)   /* empty */
# endif
#endif

/* Used for unused parameters to silence gcc warnings. */
#define UNUSED __attribute__((__unused__))

/*
 * Includes
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"

/*
 * Defines
 */
#ifndef PROGNAME
#define PROGNAME "lbcd"
#endif

#ifndef PID_FILE
#define PID_FILE "/var/run/lbcd.pid"
#endif

/*
 * Prototypes
 */

#ifdef __cplusplus
extern "C" {
#endif

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
extern void util_start_daemon(void);
extern void util_log_close(void);

/* weight.c */
int lbcd_default_weight(P_LB_RESPONSE *lb, u_int *weight, u_int *incr);
int lbcd_weight_init(const char *cmd, const char *service, int timeout);
void lbcd_setweight(P_LB_RESPONSE *lb, int offset, const char *service);

#ifdef __cplusplus
}
#endif

#endif /* __LBCD_H__ */
