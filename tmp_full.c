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
#include <portable/system.h>

#ifdef HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#elif HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif

#include <lbcd.h>

#ifndef USER_FBLOCKS
/* Percentage of the fblocks on the partition which a user may use. */
# if defined(__sgi__) || defined(_AIX)
#  define USER_FBLOCKS 1.0
# else
#  define USER_FBLOCKS 0.9
# endif
#endif


/*
 * Returns the percent full of the file system at the given path.
 */
int
tmp_full(const char *path)
{
    int percent = 0;

#if defined(HAVE_SYS_STATVFS_H) || defined(HAVE_SYS_VFS_H)
    if (chdir(path) == 0) {
# ifdef HAVE_SYS_STATVFS_H
        struct statvfs tmp;
# elif HAVE_SYS_VFS_H
        struct statfs tmp;
# endif

# ifdef HAVE_SYS_STATVFS_H
        if (statvfs(".", &tmp) == 0) {
# else
        if (statfs(".", &tmp) == 0) {
# endif
            float pct = ((tmp.f_blocks * USER_FBLOCKS) - tmp.f_bavail) * 100 /
                (tmp.f_blocks * USER_FBLOCKS);
            percent = pct + 0.5;  /* round result */
        }

        /* Sanity check */
        if (percent < 0)
            percent = 0;
        else if (percent > 100)
            percent = 100;
    }
#endif
    return percent;
}


/*
 * Test routine.
 */
#ifdef MAIN
int
main(void)
{
    printf("%%%d percent tmp full.\n", tmp_full("/tmp"));
# ifdef P_tmpdir
    printf("%%%d percent P_tmpdir full.\n", tmp_full(P_tmpdir));
# endif
    return 0;
}
#endif
