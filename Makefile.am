# Automake makefile for lbcd.
#
# Written by Russ Allbery <rra@stanford.edu>
# Copyright 1996, 1997, 1998, 2003, 2004, 2005, 2006, 2008, 2009, 2012
#     The Board of Trustees of the Leland Stanford Junior University
#
# See LICENSE for licensing terms.

ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = LICENSE autogen lbcd.pod

noinst_LIBRARIES = modules/libmodules.a portable/libportable.a util/libutil.a
portable_libportable_a_SOURCES = portable/dummy.c portable/getaddrinfo.h \
        portable/getnameinfo.h portable/macros.h portable/socket.h	 \
        portable/stdbool.h portable/system.h
portable_libportable_a_LIBADD = $(LIBOBJS)
modules_libmodules_a_SOURCES = modules/check_reply.c modules/ftp.c	 \
	modules/http.c modules/imap.c modules/ldap.c modules/modules.h	 \
	modules/monlist.c modules/monlist.h modules/nntp.c modules/ntp.c \
	modules/pop.c modules/smtp.c modules/tcp.c modules/tcp_socket.c	 \
	modules/udp_socket.c
util_libutil_a_SOURCES = util/macros.h util/messages.c util/messages.h	\
	util/xmalloc.c util/xmalloc.h

sbin_PROGRAMS = lbcd
lbcd_SOURCES = get_user.c kernel.c lbcd.c lbcd.h lbcdload.h load.c \
	protocol.h server.c tmp_full.c weight.c
lbcd_LDADD = modules/libmodules.a portable/libportable.a util/libutil.a
dist_bin_SCRIPTS = lbcdclient
dist_man_MANS = lbcd.8 lbcdclient.1

$(srcdir)/lbcd.8: lbcd.pod configure.ac
	pod2man --release=$(VERSION) --center="System Daemons" \
	    --section=8 $(srcdir)/lbcd.pod > $@

$(srcdir)/lbcdclient.1: lbcdclient configure.ac
	pod2man --release=$(VERSION) --center="User Commands" \
	    --section=1 $(srcdir)/lbcdclient > $@

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
