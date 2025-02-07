#!/bin/sh
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
# CREATE:   Mon Mar 9 2015
# REVISION:
#

#--------------------------------------------------------------
# Common Variables
#--------------------------------------------------------------
#PRGNAME=$(basename "${0}")
SCRIPTDIR=$(dirname "${0}")
SCRIPTDIR=$(cd "${SCRIPTDIR}" || exit 1; pwd)
SRCTOP=$(cd "${SCRIPTDIR}/.." || exit 1; pwd)

#
# Directories / Files
#
SRCDIR="${SRCTOP}/src"
LIBDIR="${SRCTOP}/lib"
TESTDIR="${SRCTOP}/tests"

K2HTPDTORSVR_BIN="${SRCDIR}/k2htpdtorsvr"
K2HTPDTORCLIENT_BIN="${TESTDIR}/k2htpdtorclient"
K2HTPDTOR_LIB="${LIBDIR}/.libs/libk2htpdtor.so.1"

CFG_SERVER_FILE="${TESTDIR}/dtor_test_server.ini"
CFG_SLAVE_FILE="${TESTDIR}/dtor_test_slave.ini"

TEST_K2H_FILE="/tmp/k2htpdtorsvr_test.k2h"
TEST_LOG_FILE="/tmp/k2htpdtorsvr_test.log"
TEST_ARCHIVE_DATA_FILE="${TESTDIR}/dtor_test_archive.data"
TEST_BASE_RESULT_LOG="${TESTDIR}/dtor_svr_test_result.log"

#
# Others
#
CHMPX_DEBUG_LEVEL="silent"

WAIT_SEC_AFTER_RUN_PROC=2
WAIT_SEC_AFTER_RUN_TEST=10

#--------------------------------------------------------------
# Variables and Utility functions
#--------------------------------------------------------------
#
# Escape sequence
#
if [ -t 1 ]; then
	CBLD=$(printf '\033[1m')
	CREV=$(printf '\033[7m')
	CRED=$(printf '\033[31m')
	CYEL=$(printf '\033[33m')
	CGRN=$(printf '\033[32m')
	CDEF=$(printf '\033[0m')
else
	CBLD=""
	CREV=""
	CRED=""
	CYEL=""
	CGRN=""
	CDEF=""
fi

#--------------------------------------------------------------
# Message functions
#--------------------------------------------------------------
PRNTITLE()
{
	echo ""
	echo "${CGRN}---------------------------------------------------------------------${CDEF}"
	echo "${CGRN}${CREV}[TITLE]${CDEF} ${CGRN}$*${CDEF}"
	echo "${CGRN}---------------------------------------------------------------------${CDEF}"
}

PRNERR()
{
	echo "${CBLD}${CRED}[ERROR]${CDEF} ${CRED}$*${CDEF}"
}

PRNWARN()
{
	echo "${CYEL}${CREV}[WARNING]${CDEF} $*"
}

PRNMSG()
{
	echo "${CYEL}${CREV}[MSG]${CDEF} ${CYEL}$*${CDEF}"
}

PRNINFO()
{
	echo "${CREV}[INFO]${CDEF} $*"
}

PRNSUCCEED()
{
	echo "${CREV}[SUCCEED]${CDEF} $*"
}

#--------------------------------------------------------------
# Utility functions
#--------------------------------------------------------------
#
# Stop processes
#
stop_process()
{
	if [ $# -eq 0 ]; then
		return 1
	fi
	_STOP_PIDS="$*"

	_STOP_RESULT=0
	for _ONE_PID in ${_STOP_PIDS}; do
		_MAX_TRYCOUNT=10

		while [ "${_MAX_TRYCOUNT}" -gt 0 ]; do
			#
			# Check running status
			#
			# shellcheck disable=SC2009
			if ! ( ps -o pid,stat ax 2>/dev/null | grep -v 'PID' | awk '$2~/^[^Z]/ { print $1 }' | grep -q "^${_ONE_PID}$" || exit 1 && exit 0 ); then
				break
			fi
			#
			# Try HUP
			#
			kill -HUP "${_ONE_PID}" > /dev/null 2>&1
			sleep 1

			# shellcheck disable=SC2009
			if ! ( ps -o pid,stat ax 2>/dev/null | grep -v 'PID' | awk '$2~/^[^Z]/ { print $1 }' | grep -q "^${_ONE_PID}$" || exit 1 && exit 0 ); then
				break
			fi

			#
			# Try KILL
			#
			kill -KILL "${_ONE_PID}" > /dev/null 2>&1
			sleep 1

			# shellcheck disable=SC2009
			if ! ( ps -o pid,stat ax 2>/dev/null | grep -v 'PID' | awk '$2~/^[^Z]/ { print $1 }' | grep -q "^${_ONE_PID}$" || exit 1 && exit 0 ); then
				break
			fi
			_MAX_TRYCOUNT=$((_MAX_TRYCOUNT - 1))
		done

		if [ "${_MAX_TRYCOUNT}" -le 0 ]; then
			# shellcheck disable=SC2009
			if ( ps -o pid,stat ax 2>/dev/null | grep -v 'PID' | awk '$2~/^[^Z]/ { print $1 }' | grep -q "^${_ONE_PID}$" || exit 1 && exit 0 ); then
				PRNERR "Could not stop ${_ONE_PID} process"
				_STOP_RESULT=1
			else
				PRNWARN "Could not stop ${_ONE_PID} process, because it has maybe defunct status. So assume we were able to stop it."
			fi
		fi
	done
	return "${_STOP_RESULT}"
}

#==============================================================
# Special messages
#==============================================================

echo "${CYEL}----------------------------- ${CREV}[NOTICE]${CDEF}${CYEL} -----------------------------${CDEF}"	1>&2
echo ""																											1>&2
echo "                   ${CYEL}THIS SCRIPT IS NO LONGER USED${CDEF}"											1>&2
echo ""																											1>&2
echo " This test script and k2htpdtorserver program are not used no longer."									1>&2
echo " Following source code, script and log data is not used now."												1>&2
echo "    dtor_svr_test.sh"																						1>&2
echo "    k2htpdtorserver.cc"																					1>&2
echo "    dtor_test_result.log"																					1>&2
echo "    dtor_svr_test_result.log"																				1>&2
echo ""																											1>&2
echo " You can test all by dtor_test.sh script."																1>&2
echo ""																											1>&2
echo "${CYEL}---------------------------------------------------------------------${CDEF}"						1>&2


#==============================================================
# Check environments
#==============================================================
#
# Check chmpx binary
#
if ! command -v chmpx >/dev/null 2>&1; then
	PRNERR "Not found chmpx binary"
	exit 1
fi
CHMPXBIN=$(command -v chmpx | tr -d '\n')

#
# Check k2htouch tool
#
if ! command -v k2htouch >/dev/null 2>&1; then
	PRNERR "Not found k2htouch tool"
	exit 1
fi
CHMPXBIN=$(command -v k2htouch | tr -d '\n')

#==============================================================
# Run processes
#==============================================================
PRNTITLE "RUN ALL PROCESSES & TESTS"

#
# Cleanup
#
rm -f "${TEST_K2H_FILE}"
rm -f "${TEST_LOG_FILE}"

#
# Run chmpx for server
#
PRNMSG "RUN CHMPX(server)"

"${CHMPXBIN}" -conf "${CFG_SERVER_FILE}" -d "${CHMPX_DEBUG_LEVEL}" &
CHMPXSVRPID=$!
echo "CHMPX(server)             : ${CHMPXSVRPID}"
sleep "${WAIT_SEC_AFTER_RUN_PROC}"

#
# Run test server process
#
PRNMSG "RUN K2HTPDTORSVR(test server)"

"${K2HTPDTORSVR_BIN}" -chmpx "${CFG_SERVER_FILE}" -f "${TEST_K2H_FILE}" -fullmap &
TESTSVRPID=$!
echo "K2HTPDTORSVR(test server) : ${TESTSVRPID}"
sleep "${WAIT_SEC_AFTER_RUN_PROC}"

#
# Run chmpx for slave
#
PRNMSG "RUN CHMPX(slave)"

"${CHMPXBIN}" -conf "${CFG_SLAVE_FILE}" -d "${CHMPX_DEBUG_LEVEL}" &
CHMPXSLVPID=$!
echo "CHMPX(slave)              : ${CHMPXSLVPID}"
sleep "${WAIT_SEC_AFTER_RUN_PROC}"

#
# Run test client process
#
PRNMSG "RUN K2HTPDTORCLIENT(test client)"

if ! "${K2HTPDTORCLIENT_BIN}" -f "${TEST_ARCHIVE_DATA_FILE}" -l "${K2HTPDTOR_LIB}" -p "${CFG_SLAVE_FILE}" -c 1; then
	PRNERR "Failed to run ${K2HTPDTORCLIENT_BIN}"
	exit 1
fi
PRNINFO "Succeed to run ${K2HTPDTORCLIENT_BIN}"
sleep "${WAIT_SEC_AFTER_RUN_TEST}"



#
# Dump k2hash file on server
#
PRNMSG "DUMP K2HASH FILE(k2htouch)"
if ! "${K2HTOUCHBIN}" "${TEST_K2H_FILE}" list > "${TEST_LOG_FILE}" 2>/dev/null; then
	PRNERR "Failed to run ${K2HTOUCHBIN}"
	exit 1
fi
PRNINFO "Succeed to run ${K2HTOUCHBIN}"

PRNSUCCEED "RUN ALL PROCESSES & TESTS"

#==============================================================
# Run processes
#==============================================================
PRNTITLE "STOP ALL PROCESSES"

if ! stop_process "${TESTSVRPID} ${CHMPXSLVPID} ${CHMPXSVRPID}"; then
	PRNERR "Failed to stop all prcesses"
	exit 1
fi
PRNSUCCEED "STOP ALL PROCESSES"

#==============================================================
# Check test result
#==============================================================
PRNTITLE "CHECK TEST RESULT"

BASELINECNT=$(wc -l < "${TEST_BASE_RESULT_LOG}" | tr -d '\n')
BASEBYTECNT=$(wc -c < "${TEST_BASE_RESULT_LOG}" | tr -d '\n')
LOGLINECNT=$(wc -l < "${TEST_LOG_FILE}" | tr -d '\n')
LOGBYTECNT=$(wc -c < "${TEST_LOG_FILE}" | tr -d '\n')

if [ -z "${BASELINECNT}" ] || [ -z "${BASEBYTECNT}" ] || [ -z "${LOGLINECNT}" ] || [ -z "${LOGBYTECNT}" ] || [ "${LOGLINECNT}" -ne "${BASELINECNT}" ] || [ "${LOGBYTECNT}" -ne "${BASEBYTECNT}" ]; then
	PRNERR "FAILED"
	echo "    Base log = line count(${BASELINECNT}), bytes(${BASEBYTECNT})"
	echo "    Test log = line count(${LOGLINECNT}), bytes(${LOGBYTECNT})"
	PRNINFO "Print ${TEST_LOG_FILE} log file"
	sed -e 's/^/    /g' "${TEST_LOG_FILE}"
	exit 1
fi
PRNSUCCEED "CHECK TEST RESULT"

#
# Cleanup
#
rm -f "${TEST_K2H_FILE}"
rm -f "${TEST_LOG_FILE}"

exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
