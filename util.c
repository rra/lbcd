/*
 * Utility functions.
 *
 * Written by Larry Schwimmer
 * Copyright 1996, 1997, 1998, 2003, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/socket.h>
#include <portable/system.h>

#include <signal.h>
#include <syslog.h>

#include <lbcd.h>

/* Whether debug mode is currently enabled. */
static int util_debug_mode = 0;

/* Whether logging has been initialized. */
static int util_log_init = 0;


/*
 * Initialize logging.
 */
static void
util_log_open(void)
{
    if (util_log_init)
        return;
    openlog(PROGNAME, LOG_PID, LOG_DAEMON);
    util_log_init = 1;
}


/*
 * Shut down logging.
 */
void
util_log_close(void)
{
    if (util_log_init)
        closelog();
}


/*
 * Log an info message.  Passes the buffer directly to syslog so that %m is
 * interpreted.
 */
void
util_log_info(const char *fmt, ...)
{
    char buffer[512];
    va_list ap;

    if (!util_log_init)
        util_log_open();
    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    if (util_debug_mode)
        fprintf(stderr, "INFO: %s\n", buffer);
    syslog(LOG_INFO, buffer);
    va_end(ap);
}


/*
 * Log an error message.  Passes the buffer directly to syslog so that %m is
 * interpreted.
 */
void
util_log_error(const char *fmt, ...)
{
    char buffer[512];
    va_list ap;

    if (!util_log_init)
        util_log_open();
    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    if (util_debug_mode)
        fprintf(stderr, "ERROR: %s\n", buffer);
    syslog(LOG_ERR, buffer);
    va_end(ap);
}


/*
 * Get the PID from a file and confirm that process still exists.  Return -1
 * if no PID file, or no daemon running, else return PID in PID file.
 */
pid_t
util_get_pid_from_file(const char *file)
{
    FILE *pid;

    pid = fopen(file,"r");
    if (pid != NULL) {
        pid_t the_pid = 0;

        fscanf(pid, "%d", (int *) &the_pid);
        fclose(pid);
        if (the_pid != 0 && kill(the_pid, 0) == 0)
            return the_pid;
    }
    return -1;
}


/*
 * Write the current PID to a file.  Return -1 if can't write pid file, else
 * return 0.
 */
int
util_write_pid_in_file(const char *file)
{
    FILE *pid;

    pid = fopen(file,"w");
    if (pid == NULL) {
        util_log_error("can't create pid file: %s\n", file);
        return -1;
    } else {
        fprintf(pid, "%d\n", (int) getpid());
        fclose(pid);
    }
    return 0;
}
