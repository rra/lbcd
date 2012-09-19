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
#include <util/messages.h>


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

        if (fscanf(pid, "%d", (int *) &the_pid) < 1) {
            fclose(pid);
            return -1;
        }
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
        warn("cannot create PID file %s", file);
        return -1;
    } else {
        fprintf(pid, "%d\n", (int) getpid());
        fclose(pid);
    }
    return 0;
}
