#ifndef __LBCD_H__
#define __LBCD_H__

/* kernel.c */
extern int kernel_open();     
extern int kernel_close();
extern int kernel_getload(double *l1, double *l5, double *l15);
extern int kernel_getboottime(time_t *boottime);
extern int kernel_read(off_t where, void *dest, int dest_len);

/* get_user.c */
extern int get_user_stats(int *total,
			  int *unique, int *onconsole, time_t *user_mtime);

/* tmp_free.c */
extern int tmp_free(void);

#endif /* __LBCD_H__ */
