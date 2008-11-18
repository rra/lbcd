dnl Process this file with autoconf to produce a configure script.
dnl
dnl Written by Russ Allbery <rra@stanford.edu>
dnl Based on work by Larry Schwimmer
dnl Copyright 1996, 1997, 1998, 2003, 2004, 2005, 2006, 2008
dnl     Board of Trustees, Leland Stanford Jr. University
dnl
dnl See LICENSE for licensing terms.

AC_PREREQ([2.61])
AC_INIT([lbcd], [3.3.0], [rra@stanford.edu])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.10])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_AIX
AC_GNU_SOURCE

AC_SEARCH_LIBS([kvm_open], [kvm])
AC_SEARCH_LIBS([kstat_open], [kstat])
AC_CHECK_LIB([mld], [main], [LIBS="$LIBS -lmld"])
AC_SEARCH_LIBS([nlist], [elf])
AC_SEARCH_LIBS([gethostbyname], [nsl])
AC_SEARCH_LIBS([socket], [socket], ,
    [AC_CHECK_LIB([nsl], [socket], [LIBS="$LIBS -lsocket -lnsl"], ,
        [-lsocket])])
AC_SEARCH_LIBS([inet_aton], [resolv])
AC_CHECK_HEADERS([utmp.h utmpx.h sys/statvfs.h sys/vfs.h search.h \
    sys/select.h])
AC_CHECK_TYPE([socklen_t], ,
    [AC_DEFINE([socklen_t], [int],
        [Define to int if <sys/socket.h> does not define.])],
    [#include <sys/types.h>
     #include <sys/socket.h>])
AC_CHECK_FUNCS([hsearch getutent])
AC_FUNC_ALLOCA
AC_FUNC_SETPGRP

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
