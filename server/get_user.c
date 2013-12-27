/*
 * Get statistics about logged-in users.
 *
 * Written by Larry Schwimmer
 * Updates by Russ Allbery <eagle@eyrie.org>
 * Copyright 1996, 1997, 1998, 2006, 2008, 2012, 2013
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
#if defined(HAVE_UTMPX_H)
# include <utmpx.h>
#elif defined(HAVE_UTMP_H)
# include <utmp.h>
#endif

#include <server/internal.h>
#include <util/messages.h>
#include <util/vector.h>
#include <util/xmalloc.h>

/*
 * The utmpx and utmp function sets have the same API, so we can use a set of
 * defines to fall back to the pre-utmpx versions if needed.
 */
#if !defined(HAVE_GETUTXENT) && defined(HAVE_GETUTENT)
# define utmpx       utmp
# define getutxent() getutent()
# define setutxent() setutent()
#endif

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

/* Stores the list of unique users found so far. */
static struct vector *users = NULL;


/*
 * Create the (one, global) static hash table used to track users and
 * initialize variables.
 */
static void
uniq_start(void)
{
#ifdef HAVE_HSEARCH
    hcreate(211);
#endif
    users = vector_new();
}


/*
 * Clean up.  Free the users stored in the global users variable.
 */
static void
uniq_end(void)
{
#ifdef HAVE_HSEARCH
    hdestroy();
#endif
    if (users != NULL)
        vector_free(users);
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
        vector_add(users, name);
        item.key = users->strings[users->count - 1];
        hsearch(item, ENTER);
    }
}

#else /* !HAVE_HSEARCH */

static void
uniq_add(char *name)
{
    size_t i;

    if (i = 0; i < users->count; i++)
        if (strcmp(users->strings[i], name) == 0)
            return;
    vector_add(users, name);
}

#endif /* !HAVE_HSEARCH */


/*
 * Return the count of unique users.
 */
static size_t
uniq_count(void)
{
    return users->count;
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
    char *name;
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

    /*
     * Now count the number of unique users.  There are two implementations
     * depending on whether we have getutent or have to read the utmp file
     * ourselves.
     */
    uniq_start();
#if defined(HAVE_GETUTXENT) || defined(HAVE_GETUTENT)
    {
        struct utmpx *ut;

        setutxent();
        while ((ut = getutxent()) != NULL) {
            if (ut->ut_type != USER_PROCESS)
                continue;
            (*total)++;
            if (strncmp(ut->ut_line, "console", 7) == 0)
                *on_console = 1;
            if (strncmp(ut->ut_host, ":0", 2) == 0)
                *on_console = 1;
            name = xstrndup(ut->ut_user, sizeof(ut->ut_user));
            uniq_add(name);
            free(name);
        }
        endutxent();
    }
#else
    {
        struct utmp ut;
        int fd;

        fd = open(utmp, O_RDONLY);
        if (fd < 0) {
            syswarn("cannot open %s", utmp);
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
            if (strncmp(ut->ut_host, ":0", 2) == 0)
                *on_console = 1;
            name = xstrndup(ut.ut_name, sizeof(ut.ut_name));
            uniq_add(name);
            free(name);
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
