/*
 * tmp_full.c [las 27 Oct 1996]
 *
 * A simple routine to obtain the percentage of free space in /tmp.
 * Machines with a full tmp partition are often unusable.
 *
 * Note: Does not support Ultrix and hard-codes the minfree values.
 * Note: For unsupported systems, always reports 0% full.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#elif HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#ifndef USER_FBLOCKS
/* Percentage of the fblocks on the partition which a user may use. */
#if defined(__sgi__) || defined(_AIX)
#define USER_FBLOCKS 1.0
#else
#define USER_FBLOCKS 0.9
#endif
#endif

#ifndef TMP_DIR
/* P_tmpdir is "standard" but the important user programs seem to
 * use /tmp.  A later version of the protocol should probably hand
 * back values for both directories, but for now, we'll default
 * to /tmp.
 */
#define TMP_DIR "/tmp"
#endif

int
tmp_full(void)
{
  int percent = 0;

#if defined(HAVE_SYS_STATVFS_H) || defined(HAVE_SYS_VFS_H)
  if (chdir(TMP_DIR) == 0) {
#ifdef HAVE_SYS_STATVFS_H
  struct statvfs tmp;
#elif HAVE_SYS_VFS_H
  struct statfs tmp;
#endif

#ifdef HAVE_SYS_STATVFS_H
    if (statvfs(".",&tmp) == 0) {
#else
    if (statfs(".",&tmp) == 0) {
#endif
      float pct = ((tmp.f_blocks * USER_FBLOCKS) - tmp.f_bavail) * 100 /
	(tmp.f_blocks * USER_FBLOCKS);
      percent = pct+0.5;	/* round result */
    }

    /* Sanity check */
    if (percent < 0) {
      percent = 0;
    } else if (percent > 100) {
      percent = 100;
    }
  }
#endif

  return percent;
}

#ifdef MAIN
int
main(char argc, char **argv)
{
  printf("%%%d percent full.\n",tmp_full());
  return 0;
}
#endif
