#ifndef __modules_h__
#define __modules_h__

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
 * Prototypes
 */

#ifdef __cplusplus
extern "C" {
#endif

extern int lbcd_check_reply(int sd, int timeout, const char *token);

extern int probe_tcp(const char *host, const char *service, short port,
		     const char *replycheck, int timeout);

extern int tcp_connect(const char *host, const char *protocol, int port);
extern int udp_connect(const char *host, const char *protocol, int port);

#ifdef __cplusplus
}
#endif

#endif /* __modules_h__ */
