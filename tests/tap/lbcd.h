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

#ifndef TAP_LBCD_H
#define TAP_LBCD_H 1

#include <config.h>
#include <tests/tap/macros.h>

/* Opaque struct with process tracking data. */
struct process;

BEGIN_DECLS

/*
 * Start lbcd for tests that use it.  Takes any additional arguments to
 * lbcd, terminated by NULL.  The lbcd binary will be found in the source
 * tree.
 *
 * process_stop (from <tests/tap/process.h> can be called explicitly to stop
 * lbcd and clean up, but it will also be stopped automatically at the end of
 * the test program, so tests that only start and stop the server once can
 * just let cleanup happen automatically.
 */
struct process *lbcd_start(const char *arg, ...);

END_DECLS

#endif /* !TAP_REMCTL_H */
