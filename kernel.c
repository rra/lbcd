#include "config.h"
#include "lbcd.h"

#ifdef ultrix
#include "arch/ultrix.c"
#endif

#if defined(sparc)
#if defined(__svr4__)
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
#include "arch/irix.c"
#endif

#if defined(__linux__)
#include "arch/linux.c"
#endif
