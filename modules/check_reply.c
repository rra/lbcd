/*
 * Read from the network and check that it matches an expected token.
 *
 * Takes a timeout and a token to expect, reads bytes from the network equal
 * in length to the token, and checks that the network reply matches the
 * token.  Used as a helper routine for any lbcd load module that needs to
 * check a banner returned by a remote service.
 *
 * Written by Larry Schwimmer
 * Copyright 1997, 1998, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#ifdef __GNUC__
# define alloca __builtin_alloca
#elif HAVE_ALLOCA_H
# include <alloca.h>
#elif _AIX
# pragm allocal
#else
# define NO_ALLOCA
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "modules/modules.h"


/*
 * Given a socket, timeout, and token, check that one can read data matching
 * token from the socket within timeout seconds.  Returns 0 on success and -1
 * on failure.
 */
int
lbcd_check_reply(int sd, int timeout, const char *token)
{
    struct timeval tv = { timeout, 0 };
    fd_set rset;
    int retval = 0;
#ifdef NO_ALLOCA
    char buf[1024];
#else
    char *buf;
#endif
    int len;

    if (token == NULL)
        return -1;

    len = strlen(token);
#ifdef NO_ALLOCA
    if (len > sizeof(buf) - 1)
        return -1;
#else
    buf = alloca(len + 1);
    if (buf == NULL)
        return -1;
#endif

    FD_ZERO(&rset);
    FD_SET(sd, &rset);
    if (select(sd + 1, &rset, NULL, NULL, &tv) > 0) {
        buf[len] = '\0';
        if (read(sd, buf, len) > 0) {
            if (strcmp(buf, token) != 0)
                retval = -1;
        } else {
            retval = -1;
        }
    } else {
        retval = -1;
    }
    return retval;
}
