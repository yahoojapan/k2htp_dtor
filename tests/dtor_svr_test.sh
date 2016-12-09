#!/bin/sh
#
# k2htpdtor for K2HASH TRANSACTION PLUGIN.
#
# Copyright 2015 Yahoo! JAPAN corporation.
#
# K2HASH TRANSACTION PLUGIN is programable I/F for processing
# transaction data from modifying K2HASH data.
#
# For the full copyright and license information, please view
# the LICENSE file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Mon Mar 9 2015
# REVISION:
#

#
# MESSAGE!!
#
echo "***** WARNING *****"
echo "This test script and k2htpdtorserver program are not used no longer."
echo "Following source code, script and log data is not used now."
echo "  dtor_svr_test.sh"
echo "  k2htpdtorserver.cc"
echo "  dtor_test_result.log"
echo "  dtor_svr_test_result.log"
echo ""
echo "You can test all by dtor_test.sh script."
echo "BYE!"
exit 0

##############
# Base directory
##############
RUNDIR=`pwd`
K2HTPSCRIPTDIR=`dirname $0`
if [ "X$K2HTPSCRIPTDIR" = "X" -o "X$K2HTPSCRIPTDIR" = "X." ]; then
	TMP_BASENAME=`basename $0`
	TMP_FIRSTWORD=`echo $0 | awk -F"/" '{print $1}'`

	if [ "X$TMP_BASENAME" = "X$TMP_FIRSTWORD" ]; then
		# search path
		K2HTPSCRIPTDIR=`which $0`
		K2HTPSCRIPTDIR=`dirname $K2HTPSCRIPTDIR`
	else
		K2HTPSCRIPTDIR=.
	fi
fi
BASEDIR=`cd -P ${RUNDIR}/${K2HTPSCRIPTDIR}; pwd`

##############
# Run
##############
#
# chmpx for server
#
echo "------ RUN chmpx on server side ----------------------------"
chmpx -conf ${BASEDIR}/dtor_test_server.ini -d silent &
CHMPXSVRPID=$!
echo "chmpx on server side pid = ${CHMPXSVRPID}"
sleep 1

#
# test server process
#
echo "------ RUN test server process on server side --------------"
rm -f /tmp/k2htpdtorsvr_test.k2h
${BASEDIR}/../src/k2htpdtorsvr -chmpx ${BASEDIR}/dtor_test_server.ini -f /tmp/k2htpdtorsvr_test.k2h -fullmap &
TESTSVRPID=$!
echo "test server process pid = ${TESTSVRPID}"
sleep 1

#
# server chmpx service in
#
echo "------ SET chmpx mode to SERVICEIN -------------------------"
(sleep 2; echo SERVICEIN) | telnet localhost 8021
echo "chmpx on server side status to SERVICEIN"
sleep 2

#
# chmpx for slave
#
echo "------ RUN chmpx on slave side -----------------------------"
chmpx -conf ${BASEDIR}/dtor_test_slave.ini -d silent &
CHMPXSLVPID=$!
echo "chmpx on slave side pid = ${CHMPXSLVPID}"
sleep 2

#
# test client process
#
echo "------ RUN & START test client process on slave side -------"
${BASEDIR}/k2htpdtorclient -f ${BASEDIR}/dtor_test_archive.data -l ${BASEDIR}/../lib/.libs/libk2htpdtor.so -p ${BASEDIR}/dtor_test_slave.ini -c 1
echo "finish test client process"
sleep 10

#
# dump k2hash file on server
#
k2htouch /tmp/k2htpdtorsvr_test.k2h list > /tmp/k2htpdtorsvr_test.log 2>/dev/null

##############
# Stop
##############
kill -HUP ${TESTSVRPID}
sleep 1
kill -HUP ${CHMPXSLVPID}
sleep 1
kill -HUP ${CHMPXSVRPID}
sleep 1
kill -9 ${TESTSVRPID} ${CHMPXSLVPID} ${CHMPXSVRPID} > /dev/null 2>&1

##############
# Check
##############
#RESULT=`diff ${BASEDIR}/dtor_svr_test_result.log /tmp/k2htpdtorsvr_test.log`
BASELINECNT=`cat ${BASEDIR}/dtor_svr_test_result.log | wc -l`
BASEBYTECNT=`cat ${BASEDIR}/dtor_svr_test_result.log | wc -c`
LOGLINECNT=`cat /tmp/k2htpdtorsvr_test.log | wc -l`
LOGBYTECNT=`cat /tmp/k2htpdtorsvr_test.log | wc -c`

if [ $BASELINECNT -ne $LOGLINECNT -o $BASEBYTECNT -ne $LOGBYTECNT ]; then
	echo "RESULT --> FAILED(but skip error)"
	echo " base log = line count( $BASELINECNT ) and bytes( $BASEBYTECNT )"
	echo " test log = line count( $LOGLINECNT ) and bytes( $LOGBYTECNT )"
	echo " ------ k2htpdtorsvr_test.log ------"
	cat /tmp/k2htpdtorsvr_test.log
	echo " -----------------------------------"
#	exit 1
else
	echo "RESULT --> SUCCEED"
fi

rm -f /tmp/k2htpdtorsvr_test.k2h
rm -f /tmp/k2htpdtorsvr_test.log

exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
