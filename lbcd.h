#ifndef __LBCD_H__
#define __LBCD_H__

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
#define PID_FILE "/etc/lbcd.pid"
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
extern int tmp_full(char *path);

/* server.c */
extern int lbcd_recv_udp(int s, 
			 struct sockaddr_in *cli_addr, int * cli_len,
			 char *mesg, int max_mesg);

extern int lbcd_send_status(int s, 
			    struct sockaddr_in *cli_addr, int cli_len,
			    P_HEADER *request_header,
			    p_status_t pstat);
extern void lbcd_pack_info(P_LB_RESPONSE *lb, int round_robin);

/* util.c */
extern void util_log_info(char *fmt, ...);
extern void util_log_error(char *fmt, ...);
extern int util_get_pid_from_file(char *file);
extern int util_write_pid_in_file(char *file);
extern void util_start_daemon(void);
extern void util_log_close(void);

#ifdef __cplusplus
}
#endif

#endif /* __LBCD_H__ */
