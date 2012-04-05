/*
 * Get statistics about logged-in users.
 *
 * Written by Larry Schwimmer
 * Updates by Russ Allbery
 * Copyright 1996, 1997, 1998, 2006, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/system.h>

#include <fcntl.h>
#ifdef HAVE_SEARCH_H
# include <search.h>
#endif
#include <sys/stat.h>
#include <utmp.h>
#ifdef HAVE_UTMPX_H
# include <utmpx.h>
#endif

#include <lbcd.h>

/* The logged-in user database.  We want to stat it for modification time. */
#if defined(HAVE_UTMPX_H)
# ifdef UTMPX_FILE
#  define LBCD_UTMP_FILE UTMPX_FILE
# elif defined(UTMP_FILE)
#  define LBCD_UTMP_FILE UTMP_FILE
# else
#  define LBCD_UTMP_FILE "/etc/utmpx"
# endif
#elif defined(HAVE_UTMP_H)
# ifdef UTMP_FILE
#  define LBCD_UTMP_FILE UTMP_FILE
# else
#  define LBCD_UTMP_FILE "/etc/utmp"
# endif
#endif
static const char *utmp = LBCD_UTMP_FILE;

/* Should be enough for now, but should be changed to dynamically allocate. */
static char *users[512];
static size_t uniq_users = 0;


/*
 * Create the (one, global) static hash table used to track users and
 * initialize variables.
 */
static void
uniq_start(void)
{
    uniq_users = 0;
#ifdef HAVE_HSEARCH
    hcreate(211);               /* Prime number around our max user size. */
#endif
}


/*
 * Clean up.  Free the users stored in the global users variable.
 */
static void
uniq_end(void)
{
    size_t i;

#ifdef HAVE_HSEARCH
    hdestroy();
#endif
    if (uniq_users > 0)
        for (i = 0; i < uniq_users; i++) {
            free(users[i]);
            users[i] = NULL;
        }
}


/*
 * Count a given user.  See if they're already in our hash and, if not, add
 * them and count them.
 */
#ifdef HAVE_HSEARCH

static void
uniq_add(char *name)
{
    ENTRY item, *i;

    item.key = name;
    item.data = NULL;
    i = hsearch(item, FIND);
    if (i == NULL) {
        users[uniq_users] = malloc(strlen(name) + 1);
        item.key = users[uniq_users];
        strcpy(item.key, name);
        hsearch(item, ENTER);
        uniq_users++;
    }
}

#else /* !HAVE_HSEARCH */

static void
uniq_add(char *name)
{
    size_t i;

    if (uniq_users > 0) {
        for (i = 0; i < uniq_users; i++)
            if (strcmp(users[i], name) == 0)
                return;
    }
    users[uniq_users] = malloc(strlen(name) + 1);
    strcpy(users[uniq_users], name);
    uniq_users++;
}

#endif /* !HAVE_HSEARCH */


/*
 * Return the count of unique users.
 */
static size_t
uniq_count(void)
{
    return uniq_users;
}


/*
 * The public entry point.  Returns the total users, the unique users, a flag
 * indicating whether there is a user on console, and the time of the last
 * change.  Returns 0 on success and a negative value on error.
 *
 * Currently, this implementation ignores all characters after the first eight
 * when counting uniqueness.
 */
int
get_user_stats(int *total, int *uniq, int *on_console, time_t *user_mtime)
{
    char name[9];
    struct stat sbuf;
    static int last_total = 0;
    static int last_uniq = 0;
    static int last_on_console = 0;
    static time_t last_user_mtime = 0;

    *total      = 0;
    *uniq       = 0;
    *on_console = 0;
    *user_mtime = 0;

    /*
     * Stat the utmp file to see if it's changed.  If it hasn't, use our
     * static cached values to save resources.
     */
    if (stat(utmp, &sbuf) == 0)
        *user_mtime = sbuf.st_mtime;
    if (*user_mtime > 0 && *user_mtime == last_user_mtime) {
        *total      = last_total;
        *uniq       = last_uniq;
        *on_console = last_on_console;
        return 0;
    }

    /*
     * Check two common names for console devices and assume we have a user on
     * console if they're owned by a non-root user.
     */
    if (stat("/dev/console", &sbuf) == 0 && sbuf.st_uid != 0)
        *on_console = 1;
    if (stat("/dev/tty1", &sbuf) == 0 && sbuf.st_uid  != 0)
        *on_console = 1;

    /* Now count the number of unique users.  There are two implementations
     * depending on whether we have getutent or have to read the utmp file
     * ourselves.
     */
    uniq_start();
#ifdef HAVE_GETUTENT
    {
        struct utmp *ut;

        while ((ut = getutent()) != NULL) {
            if (ut->ut_type != USER_PROCESS)
                continue;
            (*total)++;
            if (strncmp(ut->ut_line, "console", 7) == 0)
                *on_console = 1;
            strncpy(name,ut->ut_user, 8);
            name[8] = '\0';
            uniq_add(name);
        }
        endutent();
    }
#else
    {
        struct utmp ut;
        int fd;

        fd = open(utmp, O_RDONLY);
        if (fd < 0) {
            util_log_error("can't open %s: %%ms", utmp);
            return -1;
        }
        while (read(fd, &ut, sizeof(ut)) > 0) {
# ifndef USER_PROCESS
            if (ut.ut_name[0] == '\0')
                continue;
# else
            if (ut.ut_type != USER_PROCESS)
                continue;
# endif
            (*total)++;
            if (strncmp(ut.ut_line, "console", 7) == 0)
                *on_console = 1;
            strncpy(name,ut.ut_name, 8);
            name[8] = 0;
            uniq_add(name);
        }
        close(fd);
    }
#endif /* !HAVE_GETUTENT */
    *uniq = uniq_count();
    uniq_end();

    /* Save our cached values. */
    last_total      = *total;
    last_uniq       = *uniq;
    last_on_console = *on_console;
    last_user_mtime = *user_mtime;

    return 0;
}


/*
 * Test routine.
 */
#ifdef MAIN
void util_log_error(char *fmt, ...) { }

int
main(void)
{
    int t, u, oc;
    time_t mtime;

    get_user_stats(&t, &u, &oc, &mtime);
    printf("total = %d  uniq = %d  on_cons = %d\n", t, u, oc);
    return 0;
}
#endif
