/*
 * weight.c
 *
 * Default and internal modules
 */
#include "lbcd.h"
#include "lbcdload.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

/*
 * Helper functions
 */
static lbcd_func_tab_t *service_to_func(char *service);
			/* return function handling service */
static int is_weights(char *service);
/* static char *service_to_port(char *service); */

/*
 * Supported Services List
 */
static
lbcd_func_tab_t service_table[] = {
  { "load", &lbcd_load_weight, LBCD_ARGLB },
  { "unknown", &lbcd_unknown_weight, LBCD_ARGNONE },

  { "ftp", &lbcd_ftp_weight, LBCD_ARGNONE },
  { "http", &lbcd_http_weight, LBCD_ARGPORT },
  { "imap", &lbcd_imap_weight, LBCD_ARGNONE },
#ifdef HAVE_LDAP
  { "ldap", &lbcd_ldap_weight, LBCD_ARGNONE },
#endif
  { "nntp", &lbcd_nntp_weight, LBCD_ARGNONE },
  { "ntp", &lbcd_ntp_weight, LBCD_ARGNONE },
  { "pop", &lbcd_pop_weight, LBCD_ARGNONE },
  { "smtp", &lbcd_smtp_weight, LBCD_ARGNONE },
  { "tcp", &lbcd_tcp_weight, LBCD_ARGPORT },
  { "rr", &lbcd_rr_weight, LBCD_ARGNONE },
  { "cmd", &lbcd_cmd_weight, LBCD_ARGNONE },

  /* Last element is NULLs */
  { {'\0'}, (weight_func_t *)0, LBCD_ARGNONE }
};

/*
 * Module globals
 */
static char *lbcd_command;
static char *lbcd_service;
static u_int default_weight;
static u_int default_increment;
static lbcd_func_tab_t* lbcd_default_functab;
static int lbcd_timeout;

int
lbcd_weight_init(char *cmd, char *service, int timeout)
{
  lbcd_command = cmd;
  lbcd_service = service;
  lbcd_timeout = timeout;

  /* Round robin with default specified */
  if (service != NULL && is_weights(service) == 0) {
    char *cp;

    cp = strchr(service,':');
    *cp++ = '\0';
    default_weight = atoi(service);
    default_increment = atoi(cp);
    lbcd_default_functab = service_to_func("rr");
  }
  /* External command */
  else if (cmd) {
    lbcd_default_functab = service_to_func("cmd");
  }
  /* Specified module */
  else {
    /* Specify simple defaults for round robin */
    default_weight = 1;
    default_increment = 1;

    /* Specify default load module */
    lbcd_default_functab = service_to_func(service ? service : "load");
  }

  return 0;
}

void
lbcd_setweight(P_LB_RESPONSE *lb, int offset, char *service)
{
  int *weight_ptr, *incr_ptr;
  lbcd_func_tab_t *functab;

  weight_ptr = &lb->weights[offset].host_weight;
  incr_ptr = &lb->weights[offset].host_incr;
  *incr_ptr = default_increment;

  functab = service_to_func(service);

  switch (functab->argument) {
  case LBCD_ARGNONE:
    functab->function(weight_ptr,incr_ptr,lbcd_timeout);
    break;
  case LBCD_ARGLB:
    functab->function(lb,weight_ptr,incr_ptr);
    break;
  case LBCD_ARGPORT:
    {
      char *cp;

      /* Obtain port name/number from service */
      cp = !strcmp(service,"default") ? lbcd_service : service;
      cp = strchr(cp, ':');
      if (cp) cp++;

      /* Call module */
      functab->function(weight_ptr,incr_ptr,lbcd_timeout,cp);
      break;
    }
  }
}

/*
 * Service module interfaces
 */
int
lbcd_rr_weight(u_int *weight_val, u_int *incr_val)
{
  *weight_val = default_weight;
  *incr_val = default_increment;
  return 0;
}

int
lbcd_cmd_weight(u_int *weight_val, u_int *incr_val)
{
  int fd[2];

  /* The following code is essentially popen(3)
   * with a timeout and without using /bin/sh
   */
  if (pipe(fd) == 0) {
    pid_t child;

    if ((child = fork()) < 0) {
      return lbcd_unknown_weight(weight_val,incr_val);
    } else if  (child == 0) { /* child */
      close(fd[0]);
      close(2);
      if (fd[1] != 1) {
	dup2(fd[1], 1);
	close(fd[1]);
      }
      execl(lbcd_command,lbcd_command,0);
      exit(1);
    } else { /* parent */
      int stat_loc;
      FILE *fp;
      char buf[128];

      close(fd[1]);
      if ((fp = fdopen(fd[0],"r")) == NULL) {
	return lbcd_unknown_weight(weight_val,incr_val);
      }
      while(waitpid(child,&stat_loc,0) < 0) {
	if (errno != EINTR) {
	  fclose(fp);
	  if (kill(SIGTERM,child) == -1)
	    kill(SIGKILL,child);
	  return lbcd_unknown_weight(weight_val,incr_val);
	}
      }
      if (WIFEXITED(stat_loc)) {
	if (WEXITSTATUS(stat_loc) != 0) {
	  fclose(fp);
	  return lbcd_unknown_weight(weight_val,incr_val);
	}
      } else {
	if (kill(SIGTERM,child) == -1)
	  kill(SIGKILL,child);
	fclose(fp);
	return lbcd_unknown_weight(weight_val,incr_val);
      }

      if (fgets(buf,sizeof(buf),fp) != NULL) {
	fclose(fp);

	if (sscanf(buf,"%d%d",weight_val,incr_val) != 2)
	  return lbcd_unknown_weight(weight_val,incr_val);
      } else {
	fclose(fp);
	return lbcd_unknown_weight(weight_val,incr_val);
      }
    }
  } else {
    return lbcd_unknown_weight(weight_val,incr_val);
  }
  return 0;
}

int
lbcd_unknown_weight(u_int *weight_val, u_int *incr_val)
{
  *weight_val = (u_int)-1;
  *incr_val = 0;
  return 0;
}

/*
 * Helper routines
 */
static int
is_weights(char *service)
{
  char *cp;
  int sawcolon;

  /* NULL string -- not a weight */
  if (!service)
    return -1;

  /*** service must be of the following form: weight:increment ***/

  /* Must begin and end with a number */
  if (!isdigit(*service) || !isdigit(service[strlen(service)-1]))
    return -1;

  /* Must only consist of digits and a colon */
  sawcolon = 0;
  for (cp = service; *cp; cp++) {
    if (*cp != ':' && !isdigit(*cp))
      return -1;
    sawcolon += *cp ==  ':';
  }
  /* Must contain only one colon */
  if (sawcolon != 1)
    return -1;

  /*** All okay ***/
  return 0;
}

static lbcd_func_tab_t*
service_to_func(char *service)
{
  lbcd_func_tab_t *stp;
  LBCD_SERVICE_REQ name;
  char *cp;

  /* Return default if service is NULL or "default" */
  if (service == NULL || strcmp("default",service) == 0)
    return lbcd_default_functab;

  /* Obtain service name portion (service:port) */
  strcpy(name,service);
  if ((cp = strchr(name,':')) !=NULL)
    *cp ='\0';

  /* Check table for exact match on service */
  for (stp = service_table; *stp->service != '\0'; stp++) {
    if (strcmp(name,stp->service) == 0)
      return stp;
  }

  /* Return unknown service */
  for (stp = service_table; *stp->service != '\0'; stp++)
    if (strcmp("unknown",stp->service) == 0)
      return stp;

  /* FIXME: fatal condition: should terminate program */
  return 0;
}
