#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include "lbcdload.h"
#include "modules/modules.h"

#ifdef PATH_LDAPSEARCH
#define LDAP_EXEC execl
#else
#define LDAP_EXEC execlp
#define PATH_LDAPSEARCH "ldapsearch"
#endif

#ifdef HAVE_LDAP
static int
probe_ldap(const char *host, int timeout)
{
  int retval = 0;
  int fd[2];
  
  if (pipe(fd) == 0) {
    pid_t child;

    if ((child = fork()) < 0) {
      return -1;
    } else if  (child == 0) { /* child */
      close(fd[0]);
      close(2);
      if (fd[1] != 1) {
	dup2(fd[1], 1);
	close(fd[1]);
      }
      LDAP_EXEC(PATH_LDAPSEARCH,
		"ldapsearch",
		"-x",
		"-LLL",
		"-h", host ? host : "localhost",
		"-b","cn=current,cn=connections,cn=monitor",
		"-s","sub","monitorCounter", (char *) 0);
      exit(1);
    } else { /* parent */
      int stat_loc;
      FILE *fp;
      char buf[128];

      close(fd[1]);
      if ((fp = fdopen(fd[0],"r")) == NULL)
	return -1;
      while(waitpid(child,&stat_loc,timeout) < 0) {
	if (errno != EINTR) {
	  fclose(fp);
	  if (kill(SIGTERM,child) == -1)
	    kill(SIGKILL,child);
	  return -1;
	}
      }
      if (WIFEXITED(stat_loc)) {
	if (WEXITSTATUS(stat_loc) != 0) {
	  fclose(fp);
	  return -1;
	}
      } else {
	if (kill(SIGTERM,child) == -1)
	  kill(SIGKILL,child);
	fclose(fp);
	return -1;
      }

      while(fgets(buf,sizeof(buf),fp) != NULL) {
	if (strncmp(buf,"monitorCounter: ",16) != 0)
	  continue;
	retval = atoi(buf+16);
	break;
      }
      fclose(fp);
    }
  } else {
    return -1;
  }
  return retval;
}

int
lbcd_ldap_weight(u_int *weight_val, u_int *incr_val UNUSED, int timeout,
                 char *portarg UNUSED)
{
  *weight_val = (u_int)probe_ldap("localhost",timeout);
  return (*weight_val == -1) ? -1 : 0;
}
#endif /* HAVE_LDAP */

#ifdef MAIN
int
main(int argc, char *argv[])
{
  int status;

  status = probe_ldap(argv[1],5);
  if (status == -1) {
    printf("ldap service not available\n");
  } else {
    printf("ldap server %s has %d connections.\n",
	   argv[1] ? argv[1] : "localhost", status);
  }
  return status;
}
#endif
