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
PRGNAME=$(basename "${0}")
SCRIPTDIR=$(dirname "${0}")
SCRIPTDIR=$(cd "${SCRIPTDIR}" || exit 1; pwd)
SRCTOP=$(cd "${SCRIPTDIR}/.." || exit 1; pwd)

#
# Directories / Files
#
SRCDIR="${SRCTOP}/src"
LIBDIR="${SRCTOP}/lib"
LIBOBJDIR="${LIBDIR}/.libs"
TESTDIR="${SRCTOP}/tests"

DTORSVRBIN="${SRCDIR}/k2htpdtorsvr"
DTORCLTBIN="${TESTDIR}/k2htpdtorclient"
DTORLIBSO="${LIBOBJDIR}/libk2htpdtor.so.1"

K2HTPDTOR_SVR_PLUGIN_TOOL="${TESTDIR}/k2htpdtorsvr_plugin.sh"
CLONE_K2HTPDTOR_SVR_PLUGIN_TOOL="/tmp/k2htpdtorsvr_plugin.sh"

CFG_SVR_FILE_PREFIX="${TESTDIR}/dtor_test_server"
CFG_SLV_FILE_PREFIX="${TESTDIR}/dtor_test_slave"
CFG_TRANS_SVR_FILE_PREFIX="${TESTDIR}/dtor_test_trans_server"
CFG_TRANS_SLV_FILE_PREFIX="${TESTDIR}/dtor_test_trans_slave"
CFG_JSON_STRING_DATA_FILE="${TESTDIR}/test_json_string.data"

K2H_TEST_FILE="/tmp/k2hftdtorsvr.k2h"
K2H_TRANS_TEST_FILE="/tmp/k2hftdtorsvr_trans.k2h"

DTOR_TEST_ARCHIVE_DATA_FILE="${TESTDIR}/dtor_test_archive.data"

TEST_CHMPX_SVR_LOG="/tmp/chmpx_server.log"
TEST_CHMPX_SLV_LOG="/tmp/chmpx_slave.log"
TEST_CHMPX_TRANS_SVR_LOG="/tmp/chmpx_trans_server.log"
TEST_CHMPX_TRANS_SLV_LOG="/tmp/chmpx_trans_slave.log"
TEST_K2HTPDTORSVR_TRANS_SVR_LOG="/tmp/k2htpdtorsvr_trans_server.log"
TEST_K2HTPDTORSVR_SVR_LOG="/tmp/k2htpdtorsvr_server.log"

DTOR_TEST_SLV_ARCHIVE_LOG_FILE="/tmp/dtor_test_slave_archive.log"
DTOR_TRANS_TEST_SLV_ARCHIVE_LOG_FILE="/tmp/dtor_test_trans_slave_archive.log"
DTOR_TEST_SVR_ARCHIVE_LOG_FILE="/tmp/k2hftdtorsvr_archive.log"
DTOR_TRANS_TEST_SVR_ARCHIVE_LOG_FILE="/tmp/k2hftdtorsvr_trans_archive.log"
DTOR_PLUGIN_TEST_LOG_FILE="/tmp/k2hftdtorsvr_plugin.log"
DTOR_PLUGIN_TRANS_TEST_LOG_FILE="/tmp/k2hftdtorsvr_trans_plugin.log"

DTOR_NP_DIR="/tmp"
DTOR_NP_FILE_PREFIX="k2htpdtorsvr_np_"

#
# Others
#
WAIT_SEC_AFTER_RUN_CHMPX_SVR=1
WAIT_SEC_AFTER_RUN_CHMPX_SLV=5
WAIT_SEC_AFTER_RUN_K2HTPDTORSVR=1
WAIT_SEC_AFTER_RUN_K2HTPDTORCLIENT=1
WAIT_SEC_AFTER_SEND_HUP=1
WAIT_SEC_AFTER_SEND_KILL=1

#
# LD_LIBRARY_PATH
#
LD_LIBRARY_PATH="${LIBOBJDIR}"
export LD_LIBRARY_PATH

#--------------------------------------------------------------
# Usage
#--------------------------------------------------------------
func_usage()
{
	echo ""
	echo "Usage: $1 { --help(-h) } { ini_conf } { yaml_conf } { json_conf } { json_string } { json_env } { silent | err | wan | msg }"
	echo ""
}

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
# Utilitiy functions
#--------------------------------------------------------------
#
# Run all processes
#
# $1:	type(ini, yaml, json, jsonstring, jsonenv)
#
run_all_processes()
{
	_JSON_ENV_MODE=0
	if [ $# -ne 1 ] || [ -z "$1" ]; then
		PRNERR "run_all_processes function parameter is wrong."
		return 1

	elif [ "$1" = "ini" ] || [ "$1" = "INI" ]; then
		CONF_OPT_STRING="-conf"
		CONF_FILE_EXT=".ini"
		CONF_OPT_SVR_PARAM="${CFG_SVR_FILE_PREFIX}${CONF_FILE_EXT}"
		CONF_OPT_SLV_PARAM="${CFG_SLV_FILE_PREFIX}${CONF_FILE_EXT}"
		CONF_OPT_TRANS_SVR_PARAM="${CFG_TRANS_SVR_FILE_PREFIX}${CONF_FILE_EXT}"
		CONF_OPT_TRANS_SLV_PARAM="${CFG_TRANS_SLV_FILE_PREFIX}${CONF_FILE_EXT}"
		CONF_DTORCLT_PARAM="${CONF_OPT_SLV_PARAM}"

	elif [ "$1" = "yaml" ] || [ "$1" = "YAML" ]; then
		CONF_OPT_STRING="-conf"
		CONF_FILE_EXT=".yaml"
		CONF_OPT_SVR_PARAM="${CFG_SVR_FILE_PREFIX}${CONF_FILE_EXT}"
		CONF_OPT_SLV_PARAM="${CFG_SLV_FILE_PREFIX}${CONF_FILE_EXT}"
		CONF_OPT_TRANS_SVR_PARAM="${CFG_TRANS_SVR_FILE_PREFIX}${CONF_FILE_EXT}"
		CONF_OPT_TRANS_SLV_PARAM="${CFG_TRANS_SLV_FILE_PREFIX}${CONF_FILE_EXT}"
		CONF_DTORCLT_PARAM="${CONF_OPT_SLV_PARAM}"

	elif [ "$1" = "json" ] || [ "$1" = "JSON" ]; then
		CONF_OPT_STRING="-conf"
		CONF_FILE_EXT=".json"
		CONF_OPT_SVR_PARAM="${CFG_SVR_FILE_PREFIX}${CONF_FILE_EXT}"
		CONF_OPT_SLV_PARAM="${CFG_SLV_FILE_PREFIX}${CONF_FILE_EXT}"
		CONF_OPT_TRANS_SVR_PARAM="${CFG_TRANS_SVR_FILE_PREFIX}${CONF_FILE_EXT}"
		CONF_OPT_TRANS_SLV_PARAM="${CFG_TRANS_SLV_FILE_PREFIX}${CONF_FILE_EXT}"
		CONF_DTORCLT_PARAM="${CONF_OPT_SLV_PARAM}"

	elif [ "$1" = "jsonstring" ] || [ "$1" = "JSONSTRING" ] || [ "$1" = "sjson" ] || [ "$1" = "SJSON" ] || [ "$1" = "jsonstr" ] || [ "$1" = "JSONSTR" ]; then
		CONF_OPT_STRING="-json"
		CONF_OPT_SVR_PARAM="${DTOR_TEST_SERVER_JSON_STR}"
		CONF_OPT_SLV_PARAM="${DTOR_TEST_SLAVE_JSON_STR}"
		CONF_OPT_TRANS_SVR_PARAM="${DTOR_TEST_TRANS_SERVER_JSON_STR}"
		CONF_OPT_TRANS_SLV_PARAM="${DTOR_TEST_TRANS_SLAVE_JSON_STR}"
		CONF_DTORCLT_PARAM="${DTOR_TEST_SLAVE_JSON_STR}"

	elif [ "$1" = "jsonenv" ] || [ "$1" = "JSONENV" ] || [ "$1" = "ejson" ] || [ "$1" = "EJSON" ]; then
		_JSON_ENV_MODE=1
		CONF_OPT_SVR_PARAM="${DTOR_TEST_SERVER_JSON_STR}"
		CONF_OPT_SLV_PARAM="${DTOR_TEST_SLAVE_JSON_STR}"
		CONF_OPT_TRANS_SVR_PARAM="${DTOR_TEST_TRANS_SERVER_JSON_STR}"
		CONF_OPT_TRANS_SLV_PARAM="${DTOR_TEST_TRANS_SLAVE_JSON_STR}"
		CONF_DTORCLT_PARAM="${DTOR_TEST_SLAVE_JSON_STR}"

	else
		PRNERR "run_all_processes function parameter is wrong."
		return 1
	fi

	#------------------------------------------------------
	# RUN Processes
	#------------------------------------------------------
	PRNMSG "RUN ALL PROCESSES"

	#
	# Run chmpx for trans server
	#
	if [ "${_JSON_ENV_MODE}" -ne 1 ]; then
		"${CHMPXBIN}" "${CONF_OPT_STRING}" "${CONF_OPT_TRANS_SVR_PARAM}" -d "${DEBUGMODE}" > "${TEST_CHMPX_TRANS_SVR_LOG}" 2>&1 &
	else
		CHMJSONCONF="${CONF_OPT_TRANS_SVR_PARAM}" "${CHMPXBIN}" -d "${DEBUGMODE}" > "${TEST_CHMPX_TRANS_SVR_LOG}" 2>&1 &
	fi
	CHMPXSVRTRANSPID=$!
	sleep "${WAIT_SEC_AFTER_RUN_CHMPX_SVR}"
	PRNINFO "CHMPX TRANS(server)             : ${CHMPXSVRTRANSPID}"

	#
	# Run k2htpdtorsvr process on trans server
	#
	if [ "${_JSON_ENV_MODE}" -ne 1 ]; then
		"${DTORSVRBIN}" "${CONF_OPT_STRING}" "${CONF_OPT_TRANS_SVR_PARAM}" -d "${DEBUGMODE}" > "${TEST_K2HTPDTORSVR_TRANS_SVR_LOG}" 2>&1 &
	else
		DTORSVRJSONCONF="${CONF_OPT_TRANS_SVR_PARAM}" "${DTORSVRBIN}" -d "${DEBUGMODE}" > "${TEST_K2HTPDTORSVR_TRANS_SVR_LOG}" 2>&1 &
	fi
	K2HTPDTORSVRTRANSPID=$!
	sleep "${WAIT_SEC_AFTER_RUN_K2HTPDTORSVR}"
	PRNINFO "K2HTPDTORSVR TRANS(server)      : ${K2HTPDTORSVRTRANSPID}"

	#
	# Run chmpx for trans slave
	#
	if [ "${_JSON_ENV_MODE}" -ne 1 ]; then
		"${CHMPXBIN}" "${CONF_OPT_STRING}" "${CONF_OPT_TRANS_SLV_PARAM}" -d "${DEBUGMODE}" > "${TEST_CHMPX_TRANS_SLV_LOG}" 2>&1 &
	else
		CHMJSONCONF="${CONF_OPT_TRANS_SLV_PARAM}" "${CHMPXBIN}" -d "${DEBUGMODE}" > "${TEST_CHMPX_TRANS_SLV_LOG}" 2>&1 &
	fi
	CHMPXSLVTRANSPID=$!
	sleep "${WAIT_SEC_AFTER_RUN_CHMPX_SLV}"
	PRNINFO "CHMPX TRANS(slave)              : ${CHMPXSLVTRANSPID}"

	#
	# Run chmpx for server
	#
	if [ "${_JSON_ENV_MODE}" -ne 1 ]; then
		"${CHMPXBIN}" "${CONF_OPT_STRING}" "${CONF_OPT_SVR_PARAM}" -d "${DEBUGMODE}" > "${TEST_CHMPX_SVR_LOG}" 2>&1 &
	else
		CHMJSONCONF="${CONF_OPT_SVR_PARAM}" "${CHMPXBIN}" -d "${DEBUGMODE}" > "${TEST_CHMPX_SVR_LOG}" 2>&1 &
	fi
	CHMPXSVRPID=$!
	sleep "${WAIT_SEC_AFTER_RUN_CHMPX_SVR}"
	PRNINFO "CHMPX(server)                   : ${CHMPXSVRPID}"

	#
	# Run k2htpdtorsvr process on server
	#
	if [ "${_JSON_ENV_MODE}" -ne 1 ]; then
		"${DTORSVRBIN}" "${CONF_OPT_STRING}" "${CONF_OPT_SVR_PARAM}" -d "${DEBUGMODE}" > "${TEST_K2HTPDTORSVR_SVR_LOG}" 2>&1 &
	else
		DTORSVRJSONCONF="${CONF_OPT_SVR_PARAM}" "${DTORSVRBIN}" -d "${DEBUGMODE}" > "${TEST_K2HTPDTORSVR_SVR_LOG}" 2>&1 &
	fi
	K2HTPDTORSVRPID=$!
	sleep "${WAIT_SEC_AFTER_RUN_K2HTPDTORSVR}"
	PRNINFO "K2HTPDTORSVR(server)            : ${K2HTPDTORSVRPID}"

	#
	# Run chmpx for slave
	#
	if [ "${_JSON_ENV_MODE}" -ne 1 ]; then
		"${CHMPXBIN}" "${CONF_OPT_STRING}" "${CONF_OPT_SLV_PARAM}" -d "${DEBUGMODE}" > "${TEST_CHMPX_SLV_LOG}" 2>&1 &
	else
		CHMJSONCONF="${CONF_OPT_SLV_PARAM}" "${CHMPXBIN}" -d "${DEBUGMODE}" > "${TEST_CHMPX_SLV_LOG}" 2>&1 &
	fi
	CHMPXSLVPID=$!
	sleep "${WAIT_SEC_AFTER_RUN_CHMPX_SLV}"
	PRNINFO "CHMPX(slave)                    : ${CHMPXSLVPID}"

	#----------------------------------------------------------
	# Run test client process
	#----------------------------------------------------------
	PRNMSG "RUN K2HTPDTORCLIENT"

	if ! "${DTORCLTBIN}" -f "${DTOR_TEST_ARCHIVE_DATA_FILE}" -l "${DTORLIBSO}" -p "${CONF_DTORCLT_PARAM}" -c 1; then
		PRNERR "FAILED : RUN K2HTPDTORCLIENT"
	else
		PRNINFO "SUCCEED : RUN K2HTPDTORCLIENT"
	fi
	sleep "${WAIT_SEC_AFTER_RUN_K2HTPDTORCLIENT}"

	return 0
}

#
# Check result
#
check_result()
{
	PRNMSG "CHECK RESULT"

	_FOUND_ERROR=0

	#
	# Compare file
	#
	if ! cmp "${DTOR_TRANS_TEST_SLV_ARCHIVE_LOG_FILE}" "${DTOR_TEST_SLV_ARCHIVE_LOG_FILE}" > /dev/null 2>&1; then
		PRNERR "FAILED : Detected difference between \"${DTOR_TRANS_TEST_SLV_ARCHIVE_LOG_FILE}\" and \"${DTOR_TEST_SLV_ARCHIVE_LOG_FILE}\""
		_FOUND_ERROR=1
	else
		PRNINFO "SUCCEED : The same result(\"${DTOR_TRANS_TEST_SLV_ARCHIVE_LOG_FILE}\" and \"${DTOR_TEST_SLV_ARCHIVE_LOG_FILE}\")"
	fi

	#
	# Compare file
	#
	if ! cmp "${DTOR_TRANS_TEST_SVR_ARCHIVE_LOG_FILE}" "${DTOR_TEST_SVR_ARCHIVE_LOG_FILE}" > /dev/null 2>&1; then
		PRNERR "FAILED : Detected difference between \"${DTOR_TRANS_TEST_SVR_ARCHIVE_LOG_FILE}\" and \"${DTOR_TEST_SVR_ARCHIVE_LOG_FILE}\""
		_FOUND_ERROR=1
	else
		PRNINFO "SUCCEED : The same result(\"${DTOR_TRANS_TEST_SVR_ARCHIVE_LOG_FILE}\" and \"${DTOR_TEST_SVR_ARCHIVE_LOG_FILE}\")"
	fi

	#
	# Compare file
	#
	if ! diff "${DTOR_PLUGIN_TRANS_TEST_LOG_FILE}" "${DTOR_PLUGIN_TEST_LOG_FILE}" > /dev/null 2>&1; then
		PRNERR "FAILED : Detected difference between \"${DTOR_PLUGIN_TRANS_TEST_LOG_FILE}\" and \"${DTOR_PLUGIN_TEST_LOG_FILE}\""
		_FOUND_ERROR=1
	else
		PRNINFO "SUCCEED : The same result(\"${DTOR_PLUGIN_TRANS_TEST_LOG_FILE}\" and \"${DTOR_PLUGIN_TEST_LOG_FILE}\")"
	fi

	#
	# Compare file line counts
	#
	DTOR_AR_LOGCNT=$(cat -v "${DTOR_TEST_SLV_ARCHIVE_LOG_FILE}" | grep -c 'K2H_' | tr -d '\n')
	SVR_AR_LOGCNT=$(cat -v "${DTOR_TEST_SVR_ARCHIVE_LOG_FILE}" | grep -c 'K2H_' | tr -d '\n')
	PLUGIN_LOGCNT=$(grep -c -e 'DEL_KEY' -e 'SET_ALL' -e 'REP_VAL' -e 'REP_SKEY' -e 'OW_VAL' -e 'REP_ATTR' -e 'REN_KEY' "${DTOR_PLUGIN_TEST_LOG_FILE}" | tr -d '\n')

	if [ -n "${DTOR_AR_LOGCNT}" ] && [ -n "${SVR_AR_LOGCNT}" ] && [ -n "${PLUGIN_LOGCNT}" ]  && [ "${DTOR_AR_LOGCNT}" -eq "${SVR_AR_LOGCNT}" ] && [ "${DTOR_AR_LOGCNT}" -eq "${PLUGIN_LOGCNT}" ]; then
		PRNINFO "SUCCEED : Archive log line count is same"
	else
		PRNERR "FAILED : Archive log line count is different"
		print_log_detail
		_FOUND_ERROR=1
	fi

	return "${_FOUND_ERROR}"
}

#
# Stop process
#
stop_process()
{
	if [ $# -eq 0 ]; then
		return 1
	fi
	_STOP_PIDS="$*"

	#
	# Send HUP
	#
	for _ONE_PID in ${_STOP_PIDS}; do
		if ps -p "${_ONE_PID}" >/dev/null 2>&1; then
			kill -HUP "${_ONE_PID}" > /dev/null 2>&1
		fi
		sleep "${WAIT_SEC_AFTER_SEND_HUP}"
	done

	#
	# Force stop processes if existed
	#
	_STOP_RESULT=0
	for _ONE_PID in ${_STOP_PIDS}; do
		_MAX_TRYCOUNT=10

		while [ "${_MAX_TRYCOUNT}" -gt 0 ]; do
			#
			# Check running status
			#
			if ! ps -p "${_ONE_PID}" >/dev/null 2>&1; then
				break
			fi
			#
			# Send KILL
			#
			kill -KILL "${_ONE_PID}" > /dev/null 2>&1
			sleep "${WAIT_SEC_AFTER_SEND_KILL}"
			if ! ps -p "${_ONE_PID}" >/dev/null 2>&1; then
				break
			fi
			_MAX_TRYCOUNT=$((_MAX_TRYCOUNT - 1))
		done

		if [ "${_MAX_TRYCOUNT}" -le 0 ]; then
			# shellcheck disable=SC2009
			if ps -p "${_ONE_PID}" | grep -v PID | grep -q -i 'defunct'; then
				PRNWARN "Could not stop ${_ONE_PID} process, because it has defunct status. So assume we were able to stop it."
			else
				PRNERR "Could not stop ${_ONE_PID} process"
				_STOP_RESULT=1
			fi
		fi
	done

	return "${_STOP_RESULT}"
}

#
# Stop all processes
#
stop_all_processes()
{
	if ! stop_process "${K2HTPDTORSVRPID} ${CHMPXSLVPID} ${CHMPXSVRPID} ${CHMPXSLVTRANSPID} ${K2HTPDTORSVRTRANSPID} ${CHMPXSVRTRANSPID}"; then
		PRNERR "Could not stop some processes."
		return 1
	fi
	return 0
}

#
# Print detail
#
print_log_detail()
{
	echo "    [DETAIL] : Slave Archive Log"
	if [ -f "${DTOR_TEST_SLV_ARCHIVE_LOG_FILE}" ]; then
		sed -e 's/^/      /g' "${DTOR_TEST_SLV_ARCHIVE_LOG_FILE}"
	fi

	echo "    [DETAIL] : Server Archive Log"
	if [ -f "${DTOR_TEST_SVR_ARCHIVE_LOG_FILE}" ]; then
		sed -e 's/^/      /g' "${DTOR_TEST_SVR_ARCHIVE_LOG_FILE}"
	fi

	echo "    [DETAIL] : Plugin Log"
	if [ -f "${DTOR_PLUGIN_TEST_LOG_FILE}" ]; then
		sed -e 's/^/      /g' "${DTOR_PLUGIN_TEST_LOG_FILE}"
	fi

	return 0
}

#
# Cleanup files
#
cleanup_files()
{
	rm -f "${TEST_CHMPX_SVR_LOG}"
	rm -f "${TEST_CHMPX_SLV_LOG}"
	rm -f "${TEST_CHMPX_TRANS_SVR_LOG}"
	rm -f "${TEST_CHMPX_TRANS_SLV_LOG}"
	rm -f "${TEST_K2HTPDTORSVR_TRANS_SVR_LOG}"
	rm -f "${TEST_K2HTPDTORSVR_SVR_LOG}"

	rm -f "${DTOR_TEST_SLV_ARCHIVE_LOG_FILE}"
	rm -f "${DTOR_TRANS_TEST_SLV_ARCHIVE_LOG_FILE}"
	rm -f "${DTOR_TEST_SVR_ARCHIVE_LOG_FILE}"
	rm -f "${DTOR_TRANS_TEST_SVR_ARCHIVE_LOG_FILE}"
	rm -f "${DTOR_PLUGIN_TEST_LOG_FILE}"
	rm -f "${DTOR_PLUGIN_TRANS_TEST_LOG_FILE}"

	rm -f "${CLONE_K2HTPDTOR_SVR_PLUGIN_TOOL}"
	rm -f "${K2H_TEST_FILE}"
	rm -f "${K2H_TRANS_TEST_FILE}"
	rm -f "${DTOR_NP_DIR}/${DTOR_NP_FILE_PREFIX}"*

	#
	# Cleanup temporary files in /var/lib/antpickax
	#
	rm -rf /var/lib/antpickax/.fullock /var/lib/antpickax/.k2h* /var/lib/antpickax/*

	return 0
}

#--------------------------------------------------------------
# Parse options
#--------------------------------------------------------------
#
# Variables
#
IS_FOUND_TEST_TYPE_OPT=0
DO_INI_CONF=0
DO_YAML_CONF=0
DO_JSON_CONF=0
DO_JSON_STRING=0
DO_JSON_ENV=0
DEBUGMODE=""

while [ $# -ne 0 ]; do
	if [ -z "$1" ]; then
		break

	elif [ "$1" = "-h" ] || [ "$1" = "-H" ] || [ "$1" = "--help" ] || [ "$1" = "--HELP" ]; then
		func_usage "${PRGNAME}"
		exit 0

	elif [ "$1" = "ini_conf" ] || [ "$1" = "INI_CONF" ]; then
		if [ "${DO_INI_CONF}" -eq 1 ]; then
			PRNERR "Already specified \"ini_conf\" mode."
			exit 1
		fi
		DO_INI_CONF=1
		IS_FOUND_TEST_TYPE_OPT=1

	elif [ "$1" = "yaml_conf" ] || [ "$1" = "YAML_CONF" ]; then
		if [ "${DO_YAML_CONF}" -eq 1 ]; then
			PRNERR "Already specified \"yaml_conf\" mode."
			exit 1
		fi
		DO_YAML_CONF=1
		IS_FOUND_TEST_TYPE_OPT=1

	elif [ "$1" = "json_conf" ] || [ "$1" = "JSON_CONF" ]; then
		if [ "${DO_JSON_CONF}" -eq 1 ]; then
			PRNERR "Already specified \"json_conf\" mode."
			exit 1
		fi
		DO_JSON_CONF=1
		IS_FOUND_TEST_TYPE_OPT=1

	elif [ "$1" = "json_string" ] || [ "$1" = "JSON_STRING" ]; then
		if [ "${DO_JSON_STRING}" -eq 1 ]; then
			PRNERR "Already specified \"json_string\" mode."
			exit 1
		fi
		DO_JSON_STRING=1
		IS_FOUND_TEST_TYPE_OPT=1

	elif [ "$1" = "json_env" ] || [ "$1" = "JSON_ENV" ]; then
		if [ "${DO_JSON_ENV}" -eq 1 ]; then
			PRNERR "Already specified \"json_env\" mode."
			exit 1
		fi
		DO_JSON_ENV=1
		IS_FOUND_TEST_TYPE_OPT=1

	elif [ "$1" = "silent" ] || [ "$1" = "SILENT" ]; then
		if [ -n "${DEBUGMODE}" ]; then
			PRNERR "Already specified \"silent\", \"err\", \"wan\" or \"msg\" option."
			exit 1
		fi
		DEBUGMODE="silent"

	elif [ "$1" = "err" ] || [ "$1" = "ERR" ] || [ "$1" = "error" ] || [ "$1" = "ERROR" ]; then
		if [ -n "${DEBUGMODE}" ]; then
			PRNERR "Already specified \"silent\", \"err\", \"wan\" or \"msg\" option."
			exit 1
		fi
		DEBUGMODE="err"

	elif [ "$1" = "wan" ] || [ "$1" = "WAN" ] || [ "$1" = "warn" ] || [ "$1" = "WARN" ] || [ "$1" = "warning" ] || [ "$1" = "WARNING" ]; then
		if [ -n "${DEBUGMODE}" ]; then
			PRNERR "Already specified \"silent\", \"err\", \"wan\" or \"msg\" option."
			exit 1
		fi
		DEBUGMODE="wan"

	elif [ "$1" = "msg" ] || [ "$1" = "MSG" ] || [ "$1" = "message" ] || [ "$1" = "MESSAGE" ]; then
		if [ -n "${DEBUGMODE}" ]; then
			PRNERR "Already specified \"silent\", \"err\", \"wan\" or \"msg\" option."
			exit 1
		fi
		DEBUGMODE="msg"

	else
		PRNERR "Unknown option $1 specified."
		exit 1
	fi
	shift
done

#
# Check and Set values
#
if [ "${IS_FOUND_TEST_TYPE_OPT}" -eq 0 ]; then
	DO_INI_CONF=1
	DO_YAML_CONF=0
	DO_JSON_CONF=0
	DO_JSON_STRING=0
	DO_JSON_ENV=0
fi
if [ -z "${DEBUGMODE}" ]; then
	DEBUGMODE="silent"
fi

#==============================================================
# Initialize
#==============================================================
PRNTITLE "INITIALIZE BEFORE TEST"

#
# Check binary path
#
if ! CHMPXBIN=$(command -v chmpx | tr -d '\n'); then
	PRNERR "Not found chmpx binary"
	exit 1
fi

#
# Copy plugin script
#
if ! cp -p "${K2HTPDTOR_SVR_PLUGIN_TOOL}" "${CLONE_K2HTPDTOR_SVR_PLUGIN_TOOL}"; then
	PRNERR "Failed to copy ${K2HTPDTOR_SVR_PLUGIN_TOOL} file to /tmp"
	exit 1
fi

#
# JSON String data
#
DTOR_TEST_SERVER_JSON_STR=$(grep 'DTOR_TEST_SERVER_JSON=' "${CFG_JSON_STRING_DATA_FILE}" | sed -e 's/DTOR_TEST_SERVER_JSON=//g' | tr -d '\n')
DTOR_TEST_SLAVE_JSON_STR=$(grep 'DTOR_TEST_SLAVE_JSON=' "${CFG_JSON_STRING_DATA_FILE}" | sed -e 's/DTOR_TEST_SLAVE_JSON=//g' | tr -d '\n')
DTOR_TEST_TRANS_SERVER_JSON_STR=$(grep 'DTOR_TEST_TRANS_SERVER_JSON=' "${CFG_JSON_STRING_DATA_FILE}" | sed -e 's/DTOR_TEST_TRANS_SERVER_JSON=//g' | tr -d '\n')
DTOR_TEST_TRANS_SLAVE_JSON_STR=$(grep 'DTOR_TEST_TRANS_SLAVE_JSON=' "${CFG_JSON_STRING_DATA_FILE}" | sed -e 's/DTOR_TEST_TRANS_SLAVE_JSON=//g' | tr -d '\n')

#
# Change current directory
#
cd "${TESTDIR}" || exit 1

#
# Result flag
#
TEST_RESULT=0

PRNSUCCEED "INITIALIZE BEFORE TEST"

#==============================================================
# Start INI conf file
#==============================================================
if [ "${DO_INI_CONF}" -eq 1 ]; then
	PRNTITLE "TEST FOR INI CONF FILE"

	#----------------------------------------------------------
	# Cleanup
	#----------------------------------------------------------
	cleanup_files
	touch "${DTOR_PLUGIN_TEST_LOG_FILE}"
	touch "${DTOR_PLUGIN_TRANS_TEST_LOG_FILE}"

	#----------------------------------------------------------
	# RUN all test processes
	#----------------------------------------------------------
	# [NOTE]
	# The process runs in the background, so it doesn't do any error checking.
	# If it fails to start, an error will occur in subsequent tests.
	#
	if ! run_all_processes "ini"; then
		TEST_RESULT=1
	fi

	#----------------------------------------------------------
	# Stop all processes
	#----------------------------------------------------------
	PRNMSG "STOP ALL PROCESSES"

	if ! stop_all_processes; then
		PRNERR "FAILED : STOP ALL PROCESSES"
		TEST_RESULT=1
	else
		PRNINFO "SUCCEED : STOP ALL PROCESSES"
	fi

	#----------------------------------------------------------
	# Check result
	#----------------------------------------------------------
	if ! check_result; then
		TEST_RESULT=1
	fi

	#----------------------------------------------------------
	# Result
	#----------------------------------------------------------
	if [ "${TEST_RESULT}" -ne 0 ]; then
		PRNERR "FAILED: TEST FOR INI CONF FILE"
	else
		PRNSUCCEED "TEST FOR INI CONF FILE"
	fi
fi

#==============================================================
# Start YAML conf file
#==============================================================
if [ "${TEST_RESULT}" -eq 0 ] && [ "${DO_YAML_CONF}" -eq 1 ]; then
	PRNTITLE "TEST FOR YAML CONF FILE"

	#----------------------------------------------------------
	# Cleanup
	#----------------------------------------------------------
	cleanup_files
	touch "${DTOR_PLUGIN_TEST_LOG_FILE}"
	touch "${DTOR_PLUGIN_TRANS_TEST_LOG_FILE}"

	#----------------------------------------------------------
	# RUN all test processes
	#----------------------------------------------------------
	# [NOTE]
	# The process runs in the background, so it doesn't do any error checking.
	# If it fails to start, an error will occur in subsequent tests.
	#
	if ! run_all_processes "yaml"; then
		TEST_RESULT=1
	fi

	#----------------------------------------------------------
	# Stop all processes
	#----------------------------------------------------------
	PRNMSG "STOP ALL PROCESSES"

	if ! stop_all_processes; then
		PRNERR "FAILED : STOP ALL PROCESSES"
		TEST_RESULT=1
	else
		PRNINFO "SUCCEED : STOP ALL PROCESSES"
	fi

	#----------------------------------------------------------
	# Check result
	#----------------------------------------------------------
	if ! check_result; then
		TEST_RESULT=1
	fi

	#----------------------------------------------------------
	# Result
	#----------------------------------------------------------
	if [ "${TEST_RESULT}" -ne 0 ]; then
		PRNERR "FAILED: TEST FOR YAML CONF FILE"
	else
		PRNSUCCEED "TEST FOR YAML CONF FILE"
	fi
fi

#==============================================================
# Start JSON conf file
#==============================================================
if [ "${TEST_RESULT}" -eq 0 ] && [ "${DO_JSON_CONF}" -eq 1 ]; then
	PRNTITLE "TEST FOR JSON CONF FILE"

	#----------------------------------------------------------
	# Cleanup
	#----------------------------------------------------------
	cleanup_files
	touch "${DTOR_PLUGIN_TEST_LOG_FILE}"
	touch "${DTOR_PLUGIN_TRANS_TEST_LOG_FILE}"

	#----------------------------------------------------------
	# RUN all test processes
	#----------------------------------------------------------
	# [NOTE]
	# The process runs in the background, so it doesn't do any error checking.
	# If it fails to start, an error will occur in subsequent tests.
	#
	if ! run_all_processes "json"; then
		TEST_RESULT=1
	fi

	#----------------------------------------------------------
	# Stop all processes
	#----------------------------------------------------------
	PRNMSG "STOP ALL PROCESSES"

	if ! stop_all_processes; then
		PRNERR "FAILED : STOP ALL PROCESSES"
		TEST_RESULT=1
	else
		PRNINFO "SUCCEED : STOP ALL PROCESSES"
	fi

	#----------------------------------------------------------
	# Check result
	#----------------------------------------------------------
	if ! check_result; then
		TEST_RESULT=1
	fi

	#----------------------------------------------------------
	# Result
	#----------------------------------------------------------
	if [ "${TEST_RESULT}" -ne 0 ]; then
		PRNERR "FAILED: TEST FOR JSON CONF FILE"
	else
		PRNSUCCEED "TEST FOR JSON CONF FILE"
	fi
fi

#==============================================================
# Start JSON string
#==============================================================
if [ "${TEST_RESULT}" -eq 0 ] && [ "${DO_JSON_STRING}" -eq 1 ]; then
	PRNTITLE "TEST FOR JSON STRING"

	#----------------------------------------------------------
	# Cleanup
	#----------------------------------------------------------
	cleanup_files
	touch "${DTOR_PLUGIN_TEST_LOG_FILE}"
	touch "${DTOR_PLUGIN_TRANS_TEST_LOG_FILE}"

	#----------------------------------------------------------
	# RUN all test processes
	#----------------------------------------------------------
	# [NOTE]
	# The process runs in the background, so it doesn't do any error checking.
	# If it fails to start, an error will occur in subsequent tests.
	#
	if ! run_all_processes "jsonstring"; then
		TEST_RESULT=1
	fi

	#----------------------------------------------------------
	# Stop all processes
	#----------------------------------------------------------
	PRNMSG "STOP ALL PROCESSES"

	if ! stop_all_processes; then
		PRNERR "FAILED : STOP ALL PROCESSES"
		TEST_RESULT=1
	else
		PRNINFO "SUCCEED : STOP ALL PROCESSES"
	fi

	#----------------------------------------------------------
	# Check result
	#----------------------------------------------------------
	if ! check_result; then
		TEST_RESULT=1
	fi

	#----------------------------------------------------------
	# Result
	#----------------------------------------------------------
	if [ "${TEST_RESULT}" -ne 0 ]; then
		PRNERR "FAILED: TEST FOR JSON STRING"
	else
		PRNSUCCEED "TEST FOR JSON STRING"
	fi
fi

#==============================================================
# Start JSON environment
#==============================================================
if [ "${TEST_RESULT}" -eq 0 ] && [ "${DO_JSON_ENV}" -eq 1 ]; then
	PRNTITLE "TEST FOR JSON ENV"

	#----------------------------------------------------------
	# Cleanup
	#----------------------------------------------------------
	cleanup_files
	touch "${DTOR_PLUGIN_TEST_LOG_FILE}"
	touch "${DTOR_PLUGIN_TRANS_TEST_LOG_FILE}"

	#----------------------------------------------------------
	# RUN all test processes
	#----------------------------------------------------------
	# [NOTE]
	# The process runs in the background, so it doesn't do any error checking.
	# If it fails to start, an error will occur in subsequent tests.
	#
	if ! run_all_processes "jsonenv"; then
		TEST_RESULT=1
	fi

	#----------------------------------------------------------
	# Stop all processes
	#----------------------------------------------------------
	PRNMSG "STOP ALL PROCESSES"

	if ! stop_all_processes; then
		PRNERR "FAILED : STOP ALL PROCESSES"
		TEST_RESULT=1
	else
		PRNINFO "SUCCEED : STOP ALL PROCESSES"
	fi

	#----------------------------------------------------------
	# Check result
	#----------------------------------------------------------
	if ! check_result; then
		TEST_RESULT=1
	fi

	#----------------------------------------------------------
	# Result
	#----------------------------------------------------------
	if [ "${TEST_RESULT}" -ne 0 ]; then
		PRNERR "FAILED: TEST FOR JSON ENV"
	else
		PRNSUCCEED "TEST FOR JSON ENV"
	fi
fi

#==============================================================
# Finish
#==============================================================
#
# Cleanup
#
cleanup_files

if [ "${TEST_RESULT}" -ne 0 ]; then
	exit 1
fi

exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
