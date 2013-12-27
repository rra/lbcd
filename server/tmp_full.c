/*
 * Determine the percentage of free space in /tmp.
 *
 * A simple routine to obtain the percentage of free space in /tmp.  Machines
 * with a full tmp partition are often unusable.  Does not support Ultrix and
 * hard-codes the minfree values.  For unsupported systems, always reports 0%
 * full.
 *
 * Written by Larry Schwimmer
 * Copyright 1997, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/statvfs.h>
#include <portable/system.h>

#include <server/internal.h>


/*
 * Returns the percent full of the file system at the given path.  Assume,
 * just to be safe, that the user can only use 95% of the available blocks.
 * (This number, minfree, may vary, but we don't have a good way of
 * determining it.)
 */
int
tmp_full(const char *path)
{
    struct statvfs info;
    int percent = 0;
    double total;

    if (chdir(path) == 0 && statvfs(".", &info) == 0) {
        if (info.f_bavail > info.f_blocks * 0.95)
            total = info.f_blocks;
        else
            total = info.f_blocks * 0.95;
        percent = (total - info.f_bavail) * 100.0 / total + 0.5;
        if (percent < 0)
            percent = 0;
        if (percent > 100)
            percent = 100;
    }
    return percent;
}


/*
 * Test routine.
 */
#ifdef MAIN
int
main(void)
{
    printf("%%%d percent tmp full.\n", tmp_full("/"));
# ifdef P_tmpdir
    printf("%%%d percent P_tmpdir full.\n", tmp_full(P_tmpdir));
# endif
    return 0;
}
#endif
