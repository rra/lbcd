# Automake makefile for lbcd.
#
# Written by Russ Allbery <rra@stanford.edu>
# Copyright 1996, 1997, 1998, 2003, 2004, 2005, 2006, 2008
#     Board of Trustees, Leland Stanford Jr. University
#
# See LICENSE for licensing terms.

AUTOMAKE_OPTIONS = foreign subdir-objects
EXTRA_DIST = LICENSE autogen lbcd.pod

sbin_PROGRAMS = lbcd
lbcd_SOURCES = get_user.c kernel.c lbcd.c lbcd.h lbcdload.h load.c \
	protocol.h server.c tmp_full.c util.c weight.c
lbcd_LDADD = modules/libmodules.a
dist_bin_SCRIPTS = lbcdclient
dist_man_MANS = lbcd.8 lbcdclient.1

noinst_LIBRARIES = modules/libmodules.a
modules_libmodules_a_SOURCES = modules/check_reply.c modules/ftp.c \
	modules/http.c modules/imap.c modules/ldap.c modules/monlist.c \
	modules/monlist.h modules/nntp.c modules/ntp.c modules/pop.c \
	modules/smtp.c modules/tcp.c modules/tcp_socket.c modules/udp_socket.c

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

# A set of flags for warnings.	Add -O because gcc won't find some warnings
# without optimization turned on, and add -DDEBUG=1 so we'll also compile all
# debugging code and test it.
WARNINGS = -g -O -DDEBUG=1 -Wall -W -Wendif-labels -Wpointer-arith \
	-Wbad-function-cast -Wwrite-strings -Wstrict-prototypes \
	-Wmissing-prototypes -Wnested-externs -Werror

warnings:
	$(MAKE) CFLAGS='$(WARNINGS)'
