#include "config.h"
#include "kernel.h"

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
#include "arch/hpux.c"
#endif

#if defined(__irix__)
#include "arch/irix.c"
#endif

#if defined(__linux__)
#include "arch/linux.c"
#endif
