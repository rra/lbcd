#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <alloca.h>

int
lbcd_check_reply(int sd, int timeout, char *token)
{
  struct timeval tv = { timeout, 0 };
  fd_set rset;
  int retval = 0;
  char *buf;
  int len;

  if (token == NULL)
    return -1;

  len = strlen(token);
  if ((buf = (char *)alloca(len+1)) == NULL) {
    return -1;
  }

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
