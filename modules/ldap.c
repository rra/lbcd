#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <errno.h>

#ifdef PATH_LDAPSEARCH
#define LDAP_EXEC execl
#else
#define LDAP_EXEC execlp
#define PATH_LDAPSEARCH "ldapsearch"
#endif

#define TIMEOUT 5

int
probe_ldap(char *host)
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
		"-h", host ? host : "localhost",
		"-b","cn=monitor",
		"-s","base","objectclass=*");
      exit(1);
    } else { /* parent */
      int stat_loc;
      FILE *fp;
      char buf[128];

      close(fd[1]);
      if ((fp = fdopen(fd[0],"r")) == NULL)
	return -1;
      while(waitpid(child,&stat_loc,TIMEOUT) < 0) {
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
	if (strncmp(buf,"currentconnections=",19) != 0)
	  continue;
	retval = atoi(buf+19);
	break;
      }
      fclose(fp);
    }
  } else {
    return -1;
  }
  return retval;
}

#ifdef MAIN
int
main(int argc, char *argv[])
{
  int status;

  status = probe_ldap(argv[1]);
  if (status == -1) {
    printf("ldap service not available\n");
  } else {
    printf("ldap server %s has %d connections.\n",
	   argv[1] ? argv[1] : "localhost", status);
  }
  return status;
}
#endif
