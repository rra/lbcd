/*
 * Include the appropriate kernel code for the local operating system.
 *
 * Written by Larry Schwimmer
 * Copyright 1996, 1997, 1998, 2000, 2008, 2009, 2012, 2013
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>

#if defined(__SVR4)
# include "arch/solaris.c"
#elif defined(_AIX)
# include "arch/aix.c"
#elif defined(__osf__)
# include "arch/osf.c"
#elif defined(__hpux__)
/* A completely arbitrary method of determining system version */
# include <ntl.h>
# if NTL_VERSION < 1000
#  include "arch/hpux9.c"
# else
#  include "arch/hpux10.c"
# endif
#elif defined(__MACH__)
# include "arch/darwin.c"
#elif defined(sgi)
# include "arch/irix.c"
/* Currently, on FreeBSD, we rely on /proc being mounted. */
#elif defined(__linux__) || defined(__FreeBSD_kernel__)
# include "arch/linux.c"
#endif
