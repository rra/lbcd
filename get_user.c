
#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utmp.h>

#if defined(ultrix) || defined(sparc) || defined(ibm) || defined(__alpha)
#include <malloc.h>
#include <search.h>

#endif

#ifdef sparc
ENTRY *hsearch();
#endif

static char *users[500]; /* should be enough for now, change to dynamic */
static int uniq_users=0;

static void
uniq_start()
{
  uniq_users = 0;
#ifdef C_HAS_HASH
  hcreate(211);  /* nice prime number */
#endif 
}

static void
uniq_end()
{
 int i;
#ifdef C_HAS_HASH
 hdestroy();
#endif
 if (uniq_users) for (i=0; i<uniq_users; i++) {
    free(users[i]);
    users[i]=NULL;
 }
}

static void
uniq_add(char *name)
{
#ifdef C_HAS_HASH
 ENTRY item, *i;
 item.key = name;
 item.data = 0;
 i=hsearch(item,FIND);
 if (i==NULL) { 
    item.key = users[uniq_users] = (char*)malloc(strlen(name)+1);
    strcpy(item.key,name);
    hsearch(item,ENTER);
    ++uniq_users;
  }

#else /* linear search for now */

  int i;
  if (uniq_users) {
     for (i=0; i<uniq_users; i++) {
         if (strcmp(users[i],name)==0) return;
     }
  } 
  users[uniq_users] = (char *)malloc(strlen(name)+1);
  strcpy(users[uniq_users],name);
  ++uniq_users;

#endif
}

static int
uniq_count()
{
  return uniq_users;
}

int
get_user_stats(int *total,int *uniq, int *on_console,time_t *user_mtime)
{
  char line[9],name[9],host[17];
  struct utmp ut;
  struct stat sbuf;
  int fd;

  static int last_total=0,
             last_uniq=0,
             last_on_console=0;
  static time_t last_user_mtime=0;

  *total  = 0;
  *uniq   = 0;
  *on_console = 0;
  *user_mtime = 0;

  if (stat(C_UTMP,&sbuf)==0) {
       *user_mtime = sbuf.st_mtime;
  }

  if (*user_mtime == last_user_mtime) { /* used cached values */ 
     *total      = last_total;
     *uniq       = last_uniq;
     *on_console = last_on_console;
     return 0;
  }

  uniq_start();

#ifdef __alpha
{
  struct utmp *ut;
  while((ut = getutent())!=NULL) {

    if (ut->ut_type != USER_PROCESS) continue;
     ++(*total);

     if (strncmp(ut->ut_line,"console",7)==0) ++(*on_console);
     else {
       if (stat("/dev/console",&sbuf)==0) {
           if (sbuf.st_uid != 0) ++(*on_console);
       }
     }
     strncpy(name,ut->ut_user,8);
     name[8]=0;
     uniq_add(name);
  }
  endutent();
}

#else
  fd =open(C_UTMP,O_RDONLY);
  if (fd==-1) {
    util_log_error("can't open %s: %%ms",C_UTMP);
    return -1;
  }


  while (read(fd,(char*)&ut, sizeof(ut))>0) {
#ifndef ibm
     if (ut.ut_name[0] == '\0') continue;
#else
     if (ut.ut_type != USER_PROCESS) continue;
#endif
     ++(*total);
     if (strncmp(ut.ut_line,"console",7)==0) ++(*on_console);
     strncpy(name,ut.ut_name,8);
     name[8]=0;
     uniq_add(name);
  }
  close(fd);
#endif /* __alpha */

  *uniq = uniq_count();
  uniq_end();

  last_total      = *total;
  last_uniq       = *uniq; 
  last_on_console = *on_console;
  last_user_mtime = *user_mtime;

  return 0;  
}

#ifdef MAIN
int
main()
{
  int t,u,oc;
  time_t mtime;

  get_user_stats(&t,&u,&oc,&mtime);
  printf("total = %d  uniq = %d  on_cons = %d\n",t,u,oc);
  return 0;
}
#endif
