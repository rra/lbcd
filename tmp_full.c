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

int
tmp_full(const char *path)
{
  int percent = 0;

#if defined(HAVE_SYS_STATVFS_H) || defined(HAVE_SYS_VFS_H)
  if (chdir(path) == 0) {
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
  printf("%%%d percent tmp full.\n",tmp_full("/tmp"));
#ifdef P_tmpdir
  printf("%%%d percent P_tmpdir full.\n",tmp_full(P_tmpdir));
#endif
  return 0;
}
#endif
