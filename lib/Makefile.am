#
# k2htpdtor for K2HASH TRANSACTION PLUGIN.
#
# Copyright 2015 Yahoo Japan Corporation.
#
# K2HASH TRANSACTION PLUGIN is programable I/F for processing
# transaction data from modifying K2HASH data.
#
# For the full copyright and license information, please view
# the license file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Thu Feb 26 2015
# REVISION:
#

EXTRA_DIST = k2htpdtorman.h

pkgconfigdir = $(libdir)/pkgconfig

lib_LTLIBRARIES = libk2htpdtor.la

libk2htpdtor_la_SOURCES = k2htpdtor.cc k2htpdtorman.cc k2htpdtorfile.cc k2htpdtorversion.cc k2htpdtorman.h k2htpdtorfile.h
libk2htpdtor_la_LDFLAGS = -version-info $(LIB_VERSION_INFO)
libk2htpdtor_la_LIBADD = $(k2hash_LIBS) $(fullock_LIBS) $(chmpx_LIBS) -lpthread -lyaml

ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = $(k2hash_CFLAGS) $(fullock_CFLAGS) $(chmpx_CFLAGS)
AM_CPPFLAGS = $(k2hash_CFLAGS) $(fullock_CFLAGS) $(chmpx_CFLAGS)

### version(commit hash)
.PHONY: k2htpdtorversion

k2htpdtorversion.cc: k2htpdtorversion
	@../buildutils/make_commit_hash_source.sh k2htpdtorversion.cc k2htpdtor_commit_hash

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
