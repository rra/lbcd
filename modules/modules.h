/*
 * Prototypes for shared load module functions.
 *
 * Written by Russ Allbery <rra@stanford.edu>
 * Copyright 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#ifndef MODULES_MODULES_H
#define MODULES_MODULES_H 1

#include <portable/macros.h>

BEGIN_DECLS

extern int lbcd_check_reply(int sd, int timeout, const char *token);

extern int probe_tcp(const char *host, const char *service, short port,
		     const char *replycheck, int timeout);

extern int tcp_connect(const char *host, const char *protocol, int port);
extern int udp_connect(const char *host, const char *protocol, int port);

END_DECLS

#endif /* !MODULES_MODULES_H */
