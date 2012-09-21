# Automake makefile for lbcd.
#
# Written by Russ Allbery <rra@stanford.edu>
# Copyright 1996, 1997, 1998, 2003, 2004, 2005, 2006, 2008, 2009, 2012
#     The Board of Trustees of the Leland Stanford Junior University
#
# See LICENSE for licensing terms.

ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = LICENSE autogen lbcd.pod tests/README tests/TESTS		\
	tests/docs/pod-spelling-t tests/docs/pod-t tests/tap/libtap.sh	\
	tests/util/xmalloc-t

noinst_LIBRARIES = modules/libmodules.a portable/libportable.a util/libutil.a
portable_libportable_a_SOURCES = portable/dummy.c portable/getaddrinfo.h \
        portable/getnameinfo.h portable/macros.h portable/socket.h	 \
        portable/statvfs.h portable/stdbool.h portable/system.h		 \
        portable/uio.h
portable_libportable_a_LIBADD = $(LIBOBJS)
modules_libmodules_a_SOURCES = modules/check_reply.c modules/ftp.c	 \
	modules/http.c modules/imap.c modules/ldap.c modules/modules.h	 \
	modules/monlist.c modules/monlist.h modules/nntp.c modules/ntp.c \
	modules/pop.c modules/smtp.c modules/tcp.c modules/tcp_socket.c	 \
	modules/udp_socket.c
util_libutil_a_SOURCES = util/fdflag.c util/fdflag.h util/macros.h	\
	util/messages.c util/messages.h util/network.c util/network.h	\
	util/vector.c util/vector.h util/xmalloc.c util/xmalloc.h	\
	util/xwrite.c util/xwrite.h

sbin_PROGRAMS = lbcd
lbcd_SOURCES = get_user.c kernel.c internal.h lbcd.c load.c protocol.h	\
	server.c tmp_full.c weight.c
lbcd_LDADD = modules/libmodules.a util/libutil.a portable/libportable.a
dist_bin_SCRIPTS = lbcdclient
dist_man_MANS = lbcd.8 lbcdclient.1

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 build-aux/compile \
	build-aux/config.guess build-aux/config.sub build-aux/depcomp \
	build-aux/install-sh build-aux/missing config.h.in config.h.in~ \
	configure lbcd.8 lbcdclient.1

# A set of flags for warnings.  Add -O because gcc won't find some warnings
# without optimization turned on.  Desirable warnings that can't be turned
# on due to other problems:
#
#     -Wconversion      http://bugs.debian.org/488884 (htons warnings)
#
# Last checked against gcc 4.6.1 (2011-05-04).  -D_FORTIFY_SOURCE=2 enables
# warn_unused_result attribute markings on glibc functions on Linux, which
# catches a few more issues.
WARNINGS = -g -O -D_FORTIFY_SOURCE=2 -Wall -Wextra -Wendif-labels	    \
	-Wformat=2 -Winit-self -Wswitch-enum -Wdeclaration-after-statement  \
	-Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align	    \
	-Wwrite-strings -Wjump-misses-init -Wlogical-op			    \
	-Wstrict-prototypes -Wmissing-prototypes -Wredundant-decls	    \
	-Wnested-externs -Werror

warnings:
	$(MAKE) V=0 CFLAGS='$(WARNINGS)'
	$(MAKE) V=0 CFLAGS='$(WARNINGS)' $(check_PROGRAMS)

# The bits below are for the test suite, not for the main package.
check_PROGRAMS = tests/runtests tests/portable/asprintf-t		\
	tests/portable/daemon-t tests/portable/getaddrinfo-t		\
	tests/portable/getnameinfo-t tests/portable/inet_aton-t		\
	tests/portable/inet_ntoa-t tests/portable/inet_ntop-t		\
	tests/portable/snprintf-t tests/portable/strlcat-t		\
	tests/portable/strlcpy-t tests/portable/strndup-t		\
	tests/util/fdflag-t tests/util/messages-t tests/util/network-t	\
	tests/util/vector-t tests/util/xmalloc tests/util/xwrite-t
tests_runtests_CPPFLAGS = -DSOURCE='"$(abs_top_srcdir)/tests"' \
        -DBUILD='"$(abs_top_builddir)/tests"'
check_LIBRARIES = tests/tap/libtap.a
tests_tap_libtap_a_SOURCES = tests/tap/basic.c tests/tap/basic.h	\
	tests/tap/macros.h tests/tap/process.c tests/tap/process.h	\
	tests/tap/string.c tests/tap/string.h

# All of the test programs.
tests_portable_asprintf_t_SOURCES = tests/portable/asprintf-t.c \
	tests/portable/asprintf.c
tests_portable_asprintf_t_LDADD = tests/tap/libtap.a portable/libportable.a
tests_portable_daemon_t_SOURCES = tests/portable/daemon-t.c \
	tests/portable/daemon.c
tests_portable_daemon_t_LDADD = tests/tap/libtap.a portable/libportable.a
tests_portable_getaddrinfo_t_SOURCES = tests/portable/getaddrinfo-t.c \
	tests/portable/getaddrinfo.c
tests_portable_getaddrinfo_t_LDADD = tests/tap/libtap.a portable/libportable.a
tests_portable_getnameinfo_t_SOURCES = tests/portable/getnameinfo-t.c \
	tests/portable/getnameinfo.c
tests_portable_getnameinfo_t_LDADD = tests/tap/libtap.a util/libutil.a \
	portable/libportable.a
tests_portable_inet_aton_t_SOURCES = tests/portable/inet_aton-t.c \
	tests/portable/inet_aton.c
tests_portable_inet_aton_t_LDADD = tests/tap/libtap.a portable/libportable.a
tests_portable_inet_ntoa_t_SOURCES = tests/portable/inet_ntoa-t.c \
	tests/portable/inet_ntoa.c
tests_portable_inet_ntoa_t_LDADD = tests/tap/libtap.a portable/libportable.a
tests_portable_inet_ntop_t_SOURCES = tests/portable/inet_ntop-t.c \
	tests/portable/inet_ntop.c
tests_portable_inet_ntop_t_LDADD = tests/tap/libtap.a portable/libportable.a
tests_portable_snprintf_t_SOURCES = tests/portable/snprintf-t.c \
	tests/portable/snprintf.c
tests_portable_snprintf_t_LDADD = tests/tap/libtap.a portable/libportable.a
tests_portable_strlcat_t_SOURCES = tests/portable/strlcat-t.c \
	tests/portable/strlcat.c
tests_portable_strlcat_t_LDADD = tests/tap/libtap.a portable/libportable.a
tests_portable_strlcpy_t_SOURCES = tests/portable/strlcpy-t.c \
	tests/portable/strlcpy.c
tests_portable_strlcpy_t_LDADD = tests/tap/libtap.a portable/libportable.a
tests_portable_strndup_t_SOURCES = tests/portable/strndup-t.c \
	tests/portable/strndup.c
tests_portable_strndup_t_LDADD = tests/tap/libtap.a portable/libportable.a
tests_util_fdflag_t_LDADD = tests/tap/libtap.a util/libutil.a \
	portable/libportable.a
tests_util_messages_t_LDADD = tests/tap/libtap.a util/libutil.a \
	portable/libportable.a
tests_util_network_t_LDADD = tests/tap/libtap.a util/libutil.a \
	portable/libportable.a
tests_util_vector_t_LDADD = tests/tap/libtap.a util/libutil.a \
	portable/libportable.a
tests_util_xmalloc_LDADD = util/libutil.a portable/libportable.a
tests_util_xwrite_t_SOURCES = tests/util/fakewrite.c tests/util/xwrite.c \
	tests/util/xwrite-t.c
tests_util_xwrite_t_LDADD = tests/tap/libtap.a util/libutil.a \
	portable/libportable.a

check-local: $(check_PROGRAMS)
	cd tests && ./runtests $(abs_top_srcdir)/tests/TESTS
