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

bin_PROGRAMS = k2htpdtorsvr

k2htpdtorsvr_SOURCES = k2htpdtorsvr.cc k2htpdtorsvrman.cc k2htpdtorsvrplugin.cc k2htpdtorsvrfd.cc k2htpdtorsvrparser.cc k2htpdtorsvrversion.cc ../lib/k2htpdtorfile.cc k2htpdtorsvrman.h k2htpdtorsvrplugin.h k2htpdtorsvrfd.h k2htpdtorsvrparser.h
k2htpdtorsvr_LDADD = $(k2hash_LIBS) $(fullock_LIBS) $(chmpx_LIBS) -lpthread -lyaml

ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../lib $(k2hash_CFLAGS) $(fullock_CFLAGS) $(chmpx_CFLAGS)
AM_CPPFLAGS = -I../lib $(k2hash_CFLAGS) $(fullock_CFLAGS) $(chmpx_CFLAGS)

### version(commit hash)
.PHONY: k2htpdtorsvrversion

k2htpdtorsvrversion.cc: k2htpdtorsvrversion
	@../buildutils/make_commit_hash_source.sh k2htpdtorsvrversion.cc k2htpdtorsvr_commit_hash

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
