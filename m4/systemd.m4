dnl Probe for systemd libraries and installation paths.
dnl
dnl Provides the RRA_WITH_SYSTEMD_UNITDIR macro, which adds the
dnl --with-systemdsystemunitdir configure flag and sets the
dnl systemdsystemunitdir substitution variable.  Also provides the
dnl HAVE_SYSTEMD Automake conditional to use to control whether to install
dnl unit files.
dnl
dnl Also finds the compiler and linker flags for linking with the
dnl libsystemd-daemon library.  Provides the --with-libsystemd,
dnl --with-libsystemd-lib, and --with-libsystemd-include configure options to
dnl specify non-standard paths to libsystemd libraries (currently only
dnl libsystemd-daemon).
dnl
dnl Provides the macros RRA_LIB_SYSTEMD_DAEMON_OPTIONAL and sets the
dnl substitution variables SYSTEMD_DAEMON_CPPFLAGS, SYSTEMD_DAEMON_LDFLAGS,
dnl and SYSTEMD_DAEMON_LIBS.  Also provides RRA_LIB_SYSTEMD_DAEMON_SWITCH to
dnl set CPPFLAGS, LDFLAGS, and LIBS to include the systemd-daemon library,
dnl saving the current values, and RRA_LIB_SYSTEMD_DAEMON_RESTORE to restore
dnl those settings to before the last RRA_LIB_SYSTEMD_DAEMON_SWITCH.  Defines
dnl HAVE_SD_NOTIFY and sets rra_use_SYSTEMD_DAEMON to true if the library is
dnl found.
dnl
dnl Depends on the lib-helper.m4 framework.
dnl
dnl The canonical version of this file is maintained in the rra-c-util
dnl package, available at <http://www.eyrie.org/~eagle/software/rra-c-util/>.
dnl
dnl Written by Russ Allbery <eagle@eyrie.org>
dnl Copyright 2013
dnl     The Board of Trustees of the Leland Stanford Junior University
dnl
dnl This file is free software; the authors give unlimited permission to copy
dnl and/or distribute it, with or without modifications, as long as this
dnl notice is preserved.

dnl Determine the systemd system unit directory, along with a configure flag
dnl to override, and sets @systemdsystemunitdir@.  Provides the Automake
dnl HAVE_SYSTEMD Automake conditional.
AC_DEFUN([RRA_WITH_SYSTEMD_UNITDIR],
[PKG_PROG_PKG_CONFIG
 AC_ARG_WITH([systemdsystemunitdir],
    [AS_HELP_STRING([--with-systemdsystemunitdir=DIR],
        [Directory for systemd service files])],
    [],
    [with_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)])
 AS_IF([test x"$with_systemdsystemunitdir" != xno],
    [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])])
 AM_CONDITIONAL([HAVE_SYSTEMD],
    [test -n "$with_systemdsystemunitdir" -a x"$with_systemdsystemunitdir" != xno])])

dnl Save the current CPPFLAGS, LDFLAGS, and LIBS settings and switch to
dnl versions that include the systemd-daemon flags.  Used as a wrapper, with
dnl RRA_LIB_SYSTEMD_DAEMON_RESTORE, around tests.
AC_DEFUN([RRA_LIB_SYSTEMD_DAEMON_SWITCH],
[RRA_LIB_HELPER_SWITCH([SYSTEMD_DAEMON])])

dnl Restore CPPFLAGS, LDFLAGS, and LIBS to their previous values (before
dnl RRA_LIB_SYSTEMD_DAEMON_SWITCH was called).
AC_DEFUN([RRA_LIB_SYSTEMD_DAEMON_RESTORE],
[RRA_LIB_HELPER_RESTORE([SYSTEMD_DAEMON])])

dnl Checks if the libcdb library is present.  The single argument, if "true",
dnl says to fail if the libcdb library could not be found.
AC_DEFUN([_RRA_LIB_SYSTEMD_DAEMON_INTERNAL],
[RRA_LIB_HELPER_PATHS([SYSTEMD_DAEMON])
 RRA_LIB_SYSTEMD_DAEMON_SWITCH
 AC_CHECK_LIB([systemd-daemon], [sd_notify],
    [SYSTEMD_DAEMON_LIBS=-lsystemd-daemon],
    [AS_IF([test x"$1" = xtrue],
        [AC_MSG_ERROR([cannot find usable libsystemd-daemon library])])])
 AC_CHECK_HEADERS([systemd/sd-daemon.h], [],
    [SYSTEMD_DAEMON_LIBS=
     AS_IF([test x"$1" = xtrue],
        [AC_MSG_ERROR([cannot find usable systemd/sd-daemon.h header])])])
 RRA_LIB_SYSTEMD_DAEMON_RESTORE])

dnl The main macro for packages with optional systemd-daemon support.
AC_DEFUN([RRA_LIB_SYSTEMD_DAEMON_OPTIONAL],
[RRA_LIB_HELPER_VAR_INIT([SYSTEMD_DAEMON])
 RRA_LIB_HELPER_WITH_OPTIONAL([systemd], [systemd], [SYSTEMD_DAEMON])
 AS_IF([test x"$rra_use_SYSTEMD_DAEMON" != xfalse],
    [AS_IF([test x"$rra_use_SYSTEMD_DAEMON" = xtrue],
        [_RRA_LIB_SYSTEMD_DAEMON_INTERNAL([true])],
        [_RRA_LIB_SYSTEMD_DAEMON_INTERNAL([false])])])
 AS_IF([test x"$SYSTEMD_DAEMON_LIBS" != x],
    [rra_use_SYSTEMD_DAEMON=true
     AC_DEFINE([HAVE_SD_NOTIFY], 1, [Define if sd_notify is available.])])])
