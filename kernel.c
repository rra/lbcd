#include "config.h"
#include "lbcd.h"

#ifdef ultrix
#include "arch/ultrix.c"
#endif

#if defined(sun)
#if defined(__SVR4)
#include "arch/solaris.c"
#else
#include "arch/sunos.c"
#endif
#endif

#if defined(_AIX)
#include "arch/aix.c"
#endif

#if defined(__osf__)
#include "arch/osf.c"
#endif

#if defined(NeXT)
#include "arch/next.c"
#endif

#if defined(__hpux__)
/* A completely arbitrary method of determining system version */
#include <ntl.h>
#if NTL_VERSION < 1000
#include "arch/hpux9.c"
#else
#include "arch/hpux10.c"
#endif
#endif

#if defined(sgi)
#ifndef _OFF64_T
/* IRIX 5.x, IRIX 6.2 */
#include "arch/irix.c"
#else
/* > IRIX 6.2 */
#include "arch/irix6.c"
#endif
#endif

#if defined(__linux__)
#include "arch/linux.c"
#endif
