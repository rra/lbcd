#include "config.h"

#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include <netdb.h>

#include<sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif

extern int errno;

#include <signal.h>
#include <sys/param.h>

#ifdef SIGTSTP  /* true if BSD system */
#include <sys/file.h>
#include <sys/ioctl.h>
#endif

static int util_debug_mode=0;

void util_debug_on()
{
  util_debug_mode=1;
}

void util_debug_off()
{
  util_debug_mode=0;
}

void util_start_daemon()
{
 int childpid,fd;

#ifdef SIGTTOU
  signal(SIGTTOU, SIG_IGN);
#endif
#ifdef SIGTTIN
  signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTSTP
  signal(SIGTSTP, SIG_IGN);
#endif

 if ( (childpid = fork()) < 0 ) 
     util_log_error("can't fork child: %%m");
 else if (childpid > 0)
     exit(0);  /* parent */

#if defined(SIGTSTP) && defined(TIOCNTOTTY) && !defined(hpux)
  if (setpgrp(0, getpid()) == -1) 
     util_log_error("can't change process group: %%m");
  if ( (fd = open("/dev/tty", O_RDWR)) >=0) {
       ioctl(fd, TIOCNOTTY, NULL);
       close(fd);
  }
#else
  if (setpgrp() == -1)
       util_log_error("can't change process group: %%m");
  signal(SIGHUP, SIG_IGN);
  if ( (childpid = fork()) < 0) util_log_error("can't fork second child: %%m");
  else if (childpid > 0) exit(0);
#endif 

  /* close open file descriptors */
   
  for (fd=0; fd < NOFILE; fd++) close(fd);
  errno = 0;

/*  chdir("/"); */

}

static util_log_init = 0;

void util_log_open()
{

if(util_log_init) return;

#ifdef ultrix
  openlog(PROGNAME,LOG_PID);
#else
  openlog(PROGNAME,LOG_PID,LOG_DAEMON);
#endif

 util_log_init = 1;
}

void util_log_close()
{
  if ( util_log_init ) closelog();
}

void util_log_info(char *fmt, ...)
{
char buffer[512];
va_list ap;

if (!util_log_init) util_log_open();

va_start(ap, fmt);

vsprintf(buffer,fmt,ap);

if(util_debug_on) {
     fprintf(stderr,"INFO: %s\n",buffer);

va_end(argptr);
}

syslog(LOG_INFO,buffer);

va_end(ap);

}

void util_log_error(char *fmt, ...)
{
char buffer[512];
va_list ap;

if (!util_log_init) util_log_open();

va_start(ap,fmt);

vsprintf(buffer,fmt,ap);

if(util_debug_on) {
     fprintf(stderr,"ERROR: %s\n",buffer);
}

syslog(LOG_ERR,buffer);

va_end(ap);

}

unsigned int util_get_ipaddress(char *host)
{
  struct hostent *host_ent;
  struct in_addr *host_add;

  int ipaddress = inet_addr(host);

  if ( (ipaddress == -1) &&
       ( ((host_ent=gethostbyname(host)) == 0) ||
          ((host_add = (struct in_addr *) *(host_ent->h_addr_list))==0))
     )  {
          return -1 ;
       }

  if (ipaddress==-1) return host_add->s_addr;
  else return ipaddress;

}

char *util_get_host_by_address(struct in_addr in)
{
struct hostent *h;
 h=gethostbyaddr((char *) &in,sizeof(struct in_addr),AF_INET);
 if (h==NULL || h->h_name==NULL) return inet_ntoa(in);
 else return h->h_name;
}

/*
 * return -1 if no pid file, or no daemon running,
 * else return pid in pid file.
 *
 */

int util_get_pid_from_file(char *file)
{
  FILE *pid;

  pid = fopen(file,"r");
  if (pid!=NULL) {
    int the_pid;
    the_pid=0;
    fscanf(pid,"%d",&the_pid);
    fclose(pid);
    if (the_pid != 0 && kill(the_pid,0) == 0) return the_pid;
  }
  return -1;
}

/*
 * return -1 if can't write pid file, else return 0.
 *
 */

int util_write_pid_in_file(char *file)
{
  FILE *pid;

  pid = fopen(file,"w");
  if (pid==NULL) {
        util_log_error("can't create pid file: %s\n",file);
        return -1;
  } else {
      fprintf(pid,"%d\n",getpid());
      fclose(pid);
  }

  return 0;
}