#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __GNUC__
#define alloca __builtin_alloca
#elif HAVE_ALLOCA_H
#include <alloca.h>
#elif _AIX
#pragm allocal
#else
#define NO_ALLOCA
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

int
lbcd_check_reply(int sd, int timeout, char *token)
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
  if (len > sizeof(buf)-1)
    return -1;
#else
  if ((buf = (char *)alloca(len+1)) == NULL) {
    return -1;
  }
#endif

  FD_ZERO(&rset);
  FD_SET(sd,&rset);
  if (select(sd+1, &rset, NULL, NULL, &tv) > 0) {
    buf[len] = '\0';
    if (read(sd,buf,len) > 0) {
      if (strcmp(buf,token) != 0) {
	retval = -1;
      }
    } else {
      retval = -1;
    }
  } else {
    retval = -1;
  }

  return retval;
}
