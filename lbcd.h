#ifndef __LBCD_H__
#define __LBCD_H__

/*
 * Includes
 */
#include <sys/types.h>

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

/* kernel.c */
extern int kernel_getload(double *l1, double *l5, double *l15);
extern int kernel_getboottime(time_t *boottime);

/* get_user.c */
extern int get_user_stats(int *total, int *unique,
			  int *onconsole, time_t *user_mtime);

/* tmp_free.c */
extern int tmp_free(void);

#endif /* __LBCD_H__ */
