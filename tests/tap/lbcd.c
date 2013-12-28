/*
 * Spawn a copy of lbcd in the background for tests.
 *
 * Provides functions to start and stop the newly-built lbcd daemon, using
 * port 14330 instead of the default of 4330.
 *
 * Written by Russ Allbery <eagle@eyrie.org>
 * Copyright 2013
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/system.h>

#include <tests/tap/basic.h>
#include <tests/tap/lbcd.h>
#include <tests/tap/process.h>
#include <tests/tap/string.h>


/*
 * Start lbcd in the background using the process_start framework and return
 * the resulting process struct.  The path to lbcd is found relative to the
 * test directory.  Always use the -d option to avoid logging to syslog, and
 * write a PID file for synchronization as required by process_start.
 */
struct process *
lbcd_start(const char *first, ...)
{
    va_list args;
    char *tmpdir, *pidfile, *path_lbcd;
    size_t length, i;
    const char *arg, **argv;
    struct process *process;

    /* Determine the location of the PID file and detect any old one. */
    tmpdir = test_tmpdir();
    basprintf(&pidfile, "%s/lbcd.pid", tmpdir);
    if (access(pidfile, F_OK) == 0)
        bail("lbcd may already be running: %s exists", pidfile);

    /* Find the path to the newly-built lbcd binary. */
    path_lbcd = test_file_path("../server/lbcd");
    if (path_lbcd == NULL)
        bail("cannot find server/lbcd");

    /* Build the argv used to run lbcd. */
    length = 9;
    if (first != NULL) {
        length++;
        va_start(args, first);
        while ((arg = va_arg(args, const char *)) != NULL)
            length++;
    }
    va_end(args);
    argv = bmalloc(length * sizeof(const char *));
    i = 0;
    argv[i++] = path_lbcd;
    argv[i++] = "-dl";
    argv[i++] = "-p";
    argv[i++] = "14330";
    argv[i++] = "-b";
    argv[i++] = "127.0.0.1";
    argv[i++] = "-P";
    argv[i++] = pidfile;
    if (first != NULL) {
        argv[i++] = first;
        while ((arg = va_arg(args, const char *)) != NULL)
            argv[i++] = arg;
    }
    argv[i] = NULL;

    /* Start lbcd using process_start. */
    process = process_start(argv, pidfile);

    /* Clean up and return. */
    test_file_path_free(path_lbcd);
    free(pidfile);
    test_tmpdir_free(tmpdir);
    free(argv);
    return process;
}
