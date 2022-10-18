#
# k2htpdtor for K2HASH TRANSACTION PLUGIN.
#
# Copyright 2015 Yahoo Japan Corporation.
#
# K2HASH TRANSACTION PLUGIN is programmable I/F for processing
# transaction data from modifying K2HASH data.
#
# For the full copyright and license information, please view
# the license file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Tue Mar 3 2015
# REVISION:
#

noinst_PROGRAMS = k2htpdtorclient k2htpdtorserver

k2htpdtorclient_SOURCES = k2htpdtorclient.cc
k2htpdtorclient_LDADD = $(k2hash_LIBS) $(fullock_LIBS) $(chmpx_LIBS)

k2htpdtorserver_SOURCES = k2htpdtorserver.cc
k2htpdtorserver_LDADD = $(k2hash_LIBS) $(fullock_LIBS) $(chmpx_LIBS)

ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = $(k2hash_CFLAGS) $(fullock_CFLAGS) $(chmpx_CFLAGS)
AM_CPPFLAGS = $(k2hash_CFLAGS) $(fullock_CFLAGS) $(chmpx_CFLAGS)

TESTS = test.sh

EXTRA_DIST =	test.sh \
				dtor_svr_test.sh \
				k2htpdtorsvr_plugin.sh \
				dtor_test_archive.data \
				dtor_test_result.log \
				dtor_svr_test_result.log \
				dtor_test_server.ini \
				dtor_test_slave.ini \
				dtor_test_trans_server.ini \
				dtor_test_trans_slave.ini \
				dtor_test_server.yaml \
				dtor_test_slave.yaml \
				dtor_test_trans_server.yaml \
				dtor_test_trans_slave.yaml \
				dtor_test_server.json \
				dtor_test_slave.json \
				dtor_test_trans_server.json \
				dtor_test_trans_slave.json \
				test_json_string.data

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
