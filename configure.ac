dnl Process this file with autoconf to produce a configure script.
dnl $Id$

AC_REVISION([$Revision$])
AC_PREREQ(2.57)
AC_INIT([lbcd], [3.2.1], [rra@stanford.edu])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_CHECK_HEADERS([utmp.h utmpx.h sys/statvfs.h sys/vfs.h malloc.h search.h])
AC_CHECK_HEADERS([sys/select.h])

AC_SEARCH_LIBS([kvm_open], [kvm])
AC_SEARCH_LIBS([kstat_open], [kstat])
AC_CHECK_LIB([mld], [main], [LIBS="$LIBS -lmld"])
AC_SEARCH_LIBS([nlist], [elf])
AC_SEARCH_LIBS([gethostbyname], [nsl])
AC_SEARCH_LIBS([socket], [socket], ,
    [AC_CHECK_LIB([nsl], [socket], [LIBS="$LIBS -lsocket -lnsl"], ,
        [-lsocket])])

AC_CHECK_FUNCS([hsearch getutent])
AC_FUNC_ALLOCA
AC_FUNC_SETPGRP

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile modules/Makefile])
AC_OUTPUT
