#!/bin/sh
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
# CREATE:   Mon Mar 9 2015
# REVISION:
#

##############################################################
## library path & programs path
##############################################################
K2HTPSCRIPTDIR=`dirname $0`
if [ "X${SRCTOP}" = "X" ]; then
	SRCTOP=`cd ${K2HTPSCRIPTDIR}/..; pwd`
else
	K2HTPSCRIPTDIR=`cd -P ${SRTOP}/tests; pwd`
fi
K2HTPBINDIR=`cd -P ${SRCTOP}/src; pwd`
K2HTPLIBDIR=`cd -P ${SRCTOP}/lib; pwd`
cd ${K2HTPSCRIPTDIR}

if [ "X${OBJDIR}" = "X" ]; then
	LD_LIBRARY_PATH="${K2HTPLIBDIR}/.libs"
else
	LD_LIBRARY_PATH="${K2HTPLIBDIR}/${OBJDIR}"
fi
export LD_LIBRARY_PATH

#
# Binary path
#
if [ -f ${K2HTPBINDIR}/${OBJDIR}/k2htpdtorsvr ]; then
	DTORSVRBIN=${K2HTPBINDIR}/${OBJDIR}/k2htpdtorsvr
elif [ -f ${K2HTPBINDIR}/k2htpdtorsvr ]; then
	DTORSVRBIN=${K2HTPBINDIR}/k2htpdtorsvr
else
	echo "ERROR: there is no k2htpdtorsvr binary"
	echo "RESULT --> FAILED"
	exit 1
fi
if [ -f ${K2HTPSCRIPTDIR}/${OBJDIR}/k2htpdtorclient ]; then
	DTORCLTBIN=${K2HTPSCRIPTDIR}/${OBJDIR}/k2htpdtorclient
elif [ -f ${K2HTPSCRIPTDIR}/k2htpdtorclient ]; then
	DTORCLTBIN=${K2HTPSCRIPTDIR}/k2htpdtorclient
else
	echo "ERROR: there is no k2htpdtorclient binary"
	echo "RESULT --> FAILED"
	exit 1
fi
if [ -f ${K2HTPLIBDIR}/${OBJDIR}/libk2htpdtor.so.1 ]; then
	DTORLIBSO=${K2HTPLIBDIR}/${OBJDIR}/libk2htpdtor.so.1
elif [ -f ${K2HTPLIBDIR}/.libs/libk2htpdtor.so.1 ]; then
	DTORLIBSO=${K2HTPLIBDIR}/.libs/libk2htpdtor.so.1
elif [ -f ${K2HTPLIBDIR}/libk2htpdtor.so.1 ]; then
	DTORLIBSO=${K2HTPLIBDIR}/libk2htpdtor.so.1
else
	echo "ERROR: there is no libk2htpdtor.so.1 binary"
	echo "RESULT --> FAILED"
	exit 1
fi

##############################################################
## Parameters
##############################################################
#
# Check only INI type configuration as default.
#
DO_INI_CONF=yes
DO_YAML_CONF=no
DO_JSON_CONF=no
DO_JSON_STRING=no
DO_JSON_ENV=no
DEBUGMODE=silent

if [ $# -ne 0 ]; then
	DO_INI_CONF=no
	DO_YAML_CONF=no
	DO_JSON_CONF=no
	DO_JSON_STRING=no
	DO_JSON_ENV=no

	while [ $# -ne 0 ]; do
		if [ "X$1" = "Xhelp" -o "X$1" = "Xh" -o "X$1" = "X-help" -o "X$1" = "X-h" ]; then
			PROGRAM_NAME=`basename $0`
			echo "Usage: ${PROGRAM_NAME} { -help } { ini_conf } { yaml_conf } { json_conf } { json_string } { json_env } { silent | err | wan | msg }"
			echo "       no option means all process doing."
			echo ""
			exit 0
		elif [ "X$1" = "Xini_conf" ]; then
			DO_INI_CONF=yes
		elif [ "X$1" = "Xyaml_conf" ]; then
			DO_YAML_CONF=yes
		elif [ "X$1" = "Xjson_conf" ]; then
			DO_JSON_CONF=yes
		elif [ "X$1" = "Xjson_string" ]; then
			DO_JSON_STRING=yes
		elif [ "X$1" = "Xjson_env" ]; then
			DO_JSON_ENV=yes
		elif [ "X$1" = "Xsilent" ]; then
			DEBUGMODE=$1
		elif [ "X$1" = "Xerr" ]; then
			DEBUGMODE=$1
		elif [ "X$1" = "Xwan" ]; then
			DEBUGMODE=$1
		elif [ "X$1" = "Xmsg" ]; then
			DEBUGMODE=$1
		else
			echo "[ERROR] unknown option $1 specified."
			echo ""
			exit 1
		fi
		shift
	done
fi

##############################################################
# Common initialize
##############################################################
#
# copy
#
cp -p ${K2HTPSCRIPTDIR}/k2htpdtorsvr_plugin.sh /tmp/k2htpdtorsvr_plugin.sh

#
# Parameter
#
DTOR_TEST_SERVER_JSON_STR=`grep 'DTOR_TEST_SERVER_JSON=' ${K2HTPSCRIPTDIR}/test_json_string.data 2>/dev/null | sed 's/DTOR_TEST_SERVER_JSON=//g' 2>/dev/null`
DTOR_TEST_SLAVE_JSON_STR=`grep 'DTOR_TEST_SLAVE_JSON=' ${K2HTPSCRIPTDIR}/test_json_string.data 2>/dev/null | sed 's/DTOR_TEST_SLAVE_JSON=//g' 2>/dev/null`
DTOR_TEST_TRANS_SERVER_JSON_STR=`grep 'DTOR_TEST_TRANS_SERVER_JSON=' ${K2HTPSCRIPTDIR}/test_json_string.data 2>/dev/null | sed 's/DTOR_TEST_TRANS_SERVER_JSON=//g' 2>/dev/null`
DTOR_TEST_TRANS_SLAVE_JSON_STR=`grep 'DTOR_TEST_TRANS_SLAVE_JSON=' ${K2HTPSCRIPTDIR}/test_json_string.data 2>/dev/null | sed 's/DTOR_TEST_TRANS_SLAVE_JSON=//g' 2>/dev/null`

##############################################################
# Start INI conf file
##############################################################
if [ "X${DO_INI_CONF}" = "Xyes" ]; then
	echo ""
	echo ""
	echo "====== START TEST FOR INI CONF FILE ========================"
	echo ""
	CONF_FILE_EXT=".ini"

	##############
	# Initialize
	##############
	echo "------ Clear old log ---------------------------------------"
	#
	# Clear logs
	#
	rm -f /tmp/dtor_test_slave_archive.log
	rm -f /tmp/dtor_test_trans_slave_archive.log
	rm -f /tmp/k2hftdtorsvr_archive.log
	rm -f /tmp/k2hftdtorsvr_trans_archive.log
	rm -f /tmp/k2hftdtorsvr_plugin.log
	rm -f /tmp/k2hftdtorsvr_trans_plugin.log
	rm -f /tmp/k2hftdtorsvr.k2h
	rm -f /tmp/k2hftdtorsvr_trans.k2h
	rm -f /tmp/k2htpdtorsvr_np_*

	##############
	# Run
	##############
	#
	# chmpx for trans server
	#
	echo "------ RUN chmpx on trans server side ----------------------"
	chmpx -conf ${K2HTPSCRIPTDIR}/dtor_test_trans_server${CONF_FILE_EXT} -d ${DEBUGMODE} &
	CHMPXSVRTRANSPID=$!
	echo "chmpx on trans server side pid = ${CHMPXSVRTRANSPID}"
	sleep 1

	#
	# k2htpdtorsvr process on trans server
	#
	echo "------ RUN k2htpdtorsvr process on trans server ------------"
	${DTORSVRBIN} -conf ${K2HTPSCRIPTDIR}/dtor_test_trans_server${CONF_FILE_EXT} -d ${DEBUGMODE} &
	K2HTPDTORSVRTRANSPID=$!
	echo "k2htpdtorsvr process on trans server pid = ${K2HTPDTORSVRTRANSPID}"
	sleep 1

	#
	# chmpx for trans slave
	#
	echo "------ RUN chmpx on trans slave side -----------------------"
	chmpx -conf ${K2HTPSCRIPTDIR}/dtor_test_trans_slave${CONF_FILE_EXT} -d ${DEBUGMODE} &
	CHMPXSLVTRANSPID=$!
	echo "chmpx on trans slave side pid = ${CHMPXSLVTRANSPID}"
	sleep 5

	#
	# chmpx for server
	#
	echo "------ RUN chmpx on server side ----------------------------"
	chmpx -conf ${K2HTPSCRIPTDIR}/dtor_test_server${CONF_FILE_EXT} -d ${DEBUGMODE} &
	CHMPXSVRPID=$!
	echo "chmpx on server side pid = ${CHMPXSVRPID}"
	sleep 1

	#
	# k2htpdtorsvr process on server
	#
	echo "------ RUN k2htpdtorsvr process on server ------------------"
	${DTORSVRBIN} -conf ${K2HTPSCRIPTDIR}/dtor_test_server${CONF_FILE_EXT} -d ${DEBUGMODE} &
	K2HTPDTORSVRPID=$!
	echo "k2htpdtorsvr process on server pid = ${K2HTPDTORSVRPID}"
	sleep 1

	#
	# chmpx for slave
	#
	echo "------ RUN chmpx on slave side -----------------------------"
	chmpx -conf ${K2HTPSCRIPTDIR}/dtor_test_slave${CONF_FILE_EXT} -d ${DEBUGMODE} &
	CHMPXSLVPID=$!
	echo "chmpx on slave side pid = ${CHMPXSLVPID}"
	sleep 5

	#
	# test client process
	#
	echo "------ RUN & START test client process on slave side -------"
	${DTORCLTBIN} -f ${K2HTPSCRIPTDIR}/dtor_test_archive.data -l ${DTORLIBSO} -p ${K2HTPSCRIPTDIR}/dtor_test_slave${CONF_FILE_EXT} -c 1
	sleep 10

	##############
	# Stop
	##############
	echo "------ STOP all processes ----------------------------------"
	kill -HUP ${K2HTPDTORSVRPID}
	sleep 1
	kill -HUP ${CHMPXSLVPID}
	sleep 1
	kill -HUP ${CHMPXSVRPID}
	sleep 1
	kill -HUP ${CHMPXSLVTRANSPID}
	sleep 1
	kill -HUP ${K2HTPDTORSVRTRANSPID}
	sleep 1
	kill -HUP ${CHMPXSVRTRANSPID}
	sleep 1
	kill -9 ${K2HTPDTORSVRPID} ${CHMPXSLVPID} ${CHMPXSVRPID} ${CHMPXSLVTRANSPID} ${K2HTPDTORSVRTRANSPID} ${CHMPXSVRTRANSPID} > /dev/null 2>&1

	##############
	# Check
	##############
	echo "------ Check all result by testing -------------------------"
	cmp /tmp/dtor_test_trans_slave_archive.log /tmp/dtor_test_slave_archive.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare dtor_test_trans_slave_archive.log and dtor_test_slave_archive.log)"
		exit 1
	fi

	cmp /tmp/k2hftdtorsvr_trans_archive.log /tmp/k2hftdtorsvr_archive.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare k2hftdtorsvr_trans_archive.log and k2hftdtorsvr_archive.log)"
		exit 1
	fi

	diff /tmp/k2hftdtorsvr_trans_plugin.log /tmp/k2hftdtorsvr_plugin.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare k2hftdtorsvr_trans_plugin.log and k2hftdtorsvr_plugin.log)"
		exit 1
	fi

	DTOR_AR_LOGCNT=`cat -v /tmp/dtor_test_slave_archive.log | grep K2H_ | wc -l`
	SVR_AR_LOGCNT=`cat -v /tmp/k2hftdtorsvr_archive.log | grep K2H_ | wc -l`
	PLUGIN_LOGCNT=`grep -e'DEL_KEY' -e'SET_ALL' -e'REP_VAL' -e'REP_SKEY' -e'OW_VAL' -e'REP_ATTR' -e'REN_KEY' /tmp/k2hftdtorsvr_plugin.log | wc -l`

	if [ $DTOR_AR_LOGCNT -ne $SVR_AR_LOGCNT -o $DTOR_AR_LOGCNT -ne $PLUGIN_LOGCNT ]; then
		echo "RESULT --> FAILED"
		echo "(archive log line count is diffrent)"
		exit 1
	fi

	echo "INI CONF FILE TEST RESULT --> SUCCEED"
fi

##############################################################
# Start yaml conf file
##############################################################
if [ "X${DO_YAML_CONF}" = "Xyes" ]; then
	echo ""
	echo ""
	echo "====== START TEST FOR YAML CONF FILE ======================="
	echo ""
	CONF_FILE_EXT=".yaml"

	##############
	# Initialize
	##############
	echo "------ Clear old log ---------------------------------------"
	#
	# Clear logs
	#
	rm -f /tmp/dtor_test_slave_archive.log
	rm -f /tmp/dtor_test_trans_slave_archive.log
	rm -f /tmp/k2hftdtorsvr_archive.log
	rm -f /tmp/k2hftdtorsvr_trans_archive.log
	rm -f /tmp/k2hftdtorsvr_plugin.log
	rm -f /tmp/k2hftdtorsvr_trans_plugin.log
	rm -f /tmp/k2hftdtorsvr.k2h
	rm -f /tmp/k2hftdtorsvr_trans.k2h
	rm -f /tmp/k2htpdtorsvr_np_*

	##############
	# Run
	##############
	#
	# chmpx for trans server
	#
	echo "------ RUN chmpx on trans server side ----------------------"
	chmpx -conf ${K2HTPSCRIPTDIR}/dtor_test_trans_server${CONF_FILE_EXT} -d ${DEBUGMODE} &
	CHMPXSVRTRANSPID=$!
	echo "chmpx on trans server side pid = ${CHMPXSVRTRANSPID}"
	sleep 1

	#
	# k2htpdtorsvr process on trans server
	#
	echo "------ RUN k2htpdtorsvr process on trans server ------------"
	${DTORSVRBIN} -conf ${K2HTPSCRIPTDIR}/dtor_test_trans_server${CONF_FILE_EXT} -d ${DEBUGMODE} &
	K2HTPDTORSVRTRANSPID=$!
	echo "k2htpdtorsvr process on trans server pid = ${K2HTPDTORSVRTRANSPID}"
	sleep 1

	#
	# chmpx for trans slave
	#
	echo "------ RUN chmpx on trans slave side -----------------------"
	chmpx -conf ${K2HTPSCRIPTDIR}/dtor_test_trans_slave${CONF_FILE_EXT} -d ${DEBUGMODE} &
	CHMPXSLVTRANSPID=$!
	echo "chmpx on trans slave side pid = ${CHMPXSLVTRANSPID}"
	sleep 5

	#
	# chmpx for server
	#
	echo "------ RUN chmpx on server side ----------------------------"
	chmpx -conf ${K2HTPSCRIPTDIR}/dtor_test_server${CONF_FILE_EXT} -d ${DEBUGMODE} &
	CHMPXSVRPID=$!
	echo "chmpx on server side pid = ${CHMPXSVRPID}"
	sleep 1

	#
	# k2htpdtorsvr process on server
	#
	echo "------ RUN k2htpdtorsvr process on server ------------------"
	${DTORSVRBIN} -conf ${K2HTPSCRIPTDIR}/dtor_test_server${CONF_FILE_EXT} -d ${DEBUGMODE} &
	K2HTPDTORSVRPID=$!
	echo "k2htpdtorsvr process on server pid = ${K2HTPDTORSVRPID}"
	sleep 1

	#
	# chmpx for slave
	#
	echo "------ RUN chmpx on slave side -----------------------------"
	chmpx -conf ${K2HTPSCRIPTDIR}/dtor_test_slave${CONF_FILE_EXT} -d ${DEBUGMODE} &
	CHMPXSLVPID=$!
	echo "chmpx on slave side pid = ${CHMPXSLVPID}"
	sleep 5

	#
	# test client process
	#
	echo "------ RUN & START test client process on slave side -------"
	${DTORCLTBIN} -f ${K2HTPSCRIPTDIR}/dtor_test_archive.data -l ${DTORLIBSO} -p ${K2HTPSCRIPTDIR}/dtor_test_slave${CONF_FILE_EXT} -c 1
	sleep 10

	##############
	# Stop
	##############
	echo "------ STOP all processes ----------------------------------"
	kill -HUP ${K2HTPDTORSVRPID}
	sleep 1
	kill -HUP ${CHMPXSLVPID}
	sleep 1
	kill -HUP ${CHMPXSVRPID}
	sleep 1
	kill -HUP ${CHMPXSLVTRANSPID}
	sleep 1
	kill -HUP ${K2HTPDTORSVRTRANSPID}
	sleep 1
	kill -HUP ${CHMPXSVRTRANSPID}
	sleep 1
	kill -9 ${K2HTPDTORSVRPID} ${CHMPXSLVPID} ${CHMPXSVRPID} ${CHMPXSLVTRANSPID} ${K2HTPDTORSVRTRANSPID} ${CHMPXSVRTRANSPID} > /dev/null 2>&1

	##############
	# Check
	##############
	echo "------ Check all result by testing -------------------------"
	cmp /tmp/dtor_test_trans_slave_archive.log /tmp/dtor_test_slave_archive.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare dtor_test_trans_slave_archive.log and dtor_test_slave_archive.log)"
		exit 1
	fi

	cmp /tmp/k2hftdtorsvr_trans_archive.log /tmp/k2hftdtorsvr_archive.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare k2hftdtorsvr_trans_archive.log and k2hftdtorsvr_archive.log)"
		exit 1
	fi

	diff /tmp/k2hftdtorsvr_trans_plugin.log /tmp/k2hftdtorsvr_plugin.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare k2hftdtorsvr_trans_plugin.log and k2hftdtorsvr_plugin.log)"
		exit 1
	fi

	DTOR_AR_LOGCNT=`cat -v /tmp/dtor_test_slave_archive.log | grep K2H_ | wc -l`
	SVR_AR_LOGCNT=`cat -v /tmp/k2hftdtorsvr_archive.log | grep K2H_ | wc -l`
	PLUGIN_LOGCNT=`grep -e'DEL_KEY' -e'SET_ALL' -e'REP_VAL' -e'REP_SKEY' -e'OW_VAL' -e'REP_ATTR' -e'REN_KEY' /tmp/k2hftdtorsvr_plugin.log | wc -l`

	if [ $DTOR_AR_LOGCNT -ne $SVR_AR_LOGCNT -o $DTOR_AR_LOGCNT -ne $PLUGIN_LOGCNT ]; then
		echo "RESULT --> FAILED"
		echo "(archive log line count is diffrent)"
		exit 1
	fi

	echo "YAML CONF FILE TEST RESULT --> SUCCEED"
fi

##############################################################
# Start json conf file
##############################################################
if [ "X${DO_JSON_CONF}" = "Xyes" ]; then
	echo ""
	echo ""
	echo "====== START TEST FOR JSON CONF FILE ======================="
	echo ""
	CONF_FILE_EXT=".json"

	##############
	# Initialize
	##############
	echo "------ Clear old log ---------------------------------------"
	#
	# Clear logs
	#
	rm -f /tmp/dtor_test_slave_archive.log
	rm -f /tmp/dtor_test_trans_slave_archive.log
	rm -f /tmp/k2hftdtorsvr_archive.log
	rm -f /tmp/k2hftdtorsvr_trans_archive.log
	rm -f /tmp/k2hftdtorsvr_plugin.log
	rm -f /tmp/k2hftdtorsvr_trans_plugin.log
	rm -f /tmp/k2hftdtorsvr.k2h
	rm -f /tmp/k2hftdtorsvr_trans.k2h
	rm -f /tmp/k2htpdtorsvr_np_*

	##############
	# Run
	##############
	#
	# chmpx for trans server
	#
	echo "------ RUN chmpx on trans server side ----------------------"
	chmpx -conf ${K2HTPSCRIPTDIR}/dtor_test_trans_server${CONF_FILE_EXT} -d ${DEBUGMODE} &
	CHMPXSVRTRANSPID=$!
	echo "chmpx on trans server side pid = ${CHMPXSVRTRANSPID}"
	sleep 1

	#
	# k2htpdtorsvr process on trans server
	#
	echo "------ RUN k2htpdtorsvr process on trans server ------------"
	${DTORSVRBIN} -conf ${K2HTPSCRIPTDIR}/dtor_test_trans_server${CONF_FILE_EXT} -d ${DEBUGMODE} &
	K2HTPDTORSVRTRANSPID=$!
	echo "k2htpdtorsvr process on trans server pid = ${K2HTPDTORSVRTRANSPID}"
	sleep 1

	#
	# chmpx for trans slave
	#
	echo "------ RUN chmpx on trans slave side -----------------------"
	chmpx -conf ${K2HTPSCRIPTDIR}/dtor_test_trans_slave${CONF_FILE_EXT} -d ${DEBUGMODE} &
	CHMPXSLVTRANSPID=$!
	echo "chmpx on trans slave side pid = ${CHMPXSLVTRANSPID}"
	sleep 5

	#
	# chmpx for server
	#
	echo "------ RUN chmpx on server side ----------------------------"
	chmpx -conf ${K2HTPSCRIPTDIR}/dtor_test_server${CONF_FILE_EXT} -d ${DEBUGMODE} &
	CHMPXSVRPID=$!
	echo "chmpx on server side pid = ${CHMPXSVRPID}"
	sleep 1

	#
	# k2htpdtorsvr process on server
	#
	echo "------ RUN k2htpdtorsvr process on server ------------------"
	${DTORSVRBIN} -conf ${K2HTPSCRIPTDIR}/dtor_test_server${CONF_FILE_EXT} -d ${DEBUGMODE} &
	K2HTPDTORSVRPID=$!
	echo "k2htpdtorsvr process on server pid = ${K2HTPDTORSVRPID}"
	sleep 1

	#
	# chmpx for slave
	#
	echo "------ RUN chmpx on slave side -----------------------------"
	chmpx -conf ${K2HTPSCRIPTDIR}/dtor_test_slave${CONF_FILE_EXT} -d ${DEBUGMODE} &
	CHMPXSLVPID=$!
	echo "chmpx on slave side pid = ${CHMPXSLVPID}"
	sleep 5

	#
	# test client process
	#
	echo "------ RUN & START test client process on slave side -------"
	${DTORCLTBIN} -f ${K2HTPSCRIPTDIR}/dtor_test_archive.data -l ${DTORLIBSO} -p ${K2HTPSCRIPTDIR}/dtor_test_slave${CONF_FILE_EXT} -c 1
	sleep 10

	##############
	# Stop
	##############
	echo "------ STOP all processes ----------------------------------"
	kill -HUP ${K2HTPDTORSVRPID}
	sleep 1
	kill -HUP ${CHMPXSLVPID}
	sleep 1
	kill -HUP ${CHMPXSVRPID}
	sleep 1
	kill -HUP ${CHMPXSLVTRANSPID}
	sleep 1
	kill -HUP ${K2HTPDTORSVRTRANSPID}
	sleep 1
	kill -HUP ${CHMPXSVRTRANSPID}
	sleep 1
	kill -9 ${K2HTPDTORSVRPID} ${CHMPXSLVPID} ${CHMPXSVRPID} ${CHMPXSLVTRANSPID} ${K2HTPDTORSVRTRANSPID} ${CHMPXSVRTRANSPID} > /dev/null 2>&1

	##############
	# Check
	##############
	echo "------ Check all result by testing -------------------------"
	cmp /tmp/dtor_test_trans_slave_archive.log /tmp/dtor_test_slave_archive.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare dtor_test_trans_slave_archive.log and dtor_test_slave_archive.log)"
		exit 1
	fi

	cmp /tmp/k2hftdtorsvr_trans_archive.log /tmp/k2hftdtorsvr_archive.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare k2hftdtorsvr_trans_archive.log and k2hftdtorsvr_archive.log)"
		exit 1
	fi

	diff /tmp/k2hftdtorsvr_trans_plugin.log /tmp/k2hftdtorsvr_plugin.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare k2hftdtorsvr_trans_plugin.log and k2hftdtorsvr_plugin.log)"
		exit 1
	fi

	DTOR_AR_LOGCNT=`cat -v /tmp/dtor_test_slave_archive.log | grep K2H_ | wc -l`
	SVR_AR_LOGCNT=`cat -v /tmp/k2hftdtorsvr_archive.log | grep K2H_ | wc -l`
	PLUGIN_LOGCNT=`grep -e'DEL_KEY' -e'SET_ALL' -e'REP_VAL' -e'REP_SKEY' -e'OW_VAL' -e'REP_ATTR' -e'REN_KEY' /tmp/k2hftdtorsvr_plugin.log | wc -l`

	if [ $DTOR_AR_LOGCNT -ne $SVR_AR_LOGCNT -o $DTOR_AR_LOGCNT -ne $PLUGIN_LOGCNT ]; then
		echo "RESULT --> FAILED"
		echo "(archive log line count is diffrent)"
		exit 1
	fi

	echo "JSON CONF FILE TEST RESULT --> SUCCEED"
fi

##############################################################
# Start json string conf
##############################################################
if [ "X${DO_JSON_STRING}" = "Xyes" ]; then
	echo ""
	echo ""
	echo "====== START TEST FOR JSON STRING =========================="
	echo ""

	##############
	# Initialize
	##############
	echo "------ Clear old log ---------------------------------------"
	#
	# Clear logs
	#
	rm -f /tmp/dtor_test_slave_archive.log
	rm -f /tmp/dtor_test_trans_slave_archive.log
	rm -f /tmp/k2hftdtorsvr_archive.log
	rm -f /tmp/k2hftdtorsvr_trans_archive.log
	rm -f /tmp/k2hftdtorsvr_plugin.log
	rm -f /tmp/k2hftdtorsvr_trans_plugin.log
	rm -f /tmp/k2hftdtorsvr.k2h
	rm -f /tmp/k2hftdtorsvr_trans.k2h
	rm -f /tmp/k2htpdtorsvr_np_*

	##############
	# Run
	##############
	#
	# chmpx for trans server
	#
	echo "------ RUN chmpx on trans server side ----------------------"
	chmpx -json "${DTOR_TEST_TRANS_SERVER_JSON_STR}" -d ${DEBUGMODE} &
	CHMPXSVRTRANSPID=$!
	echo "chmpx on trans server side pid = ${CHMPXSVRTRANSPID}"
	sleep 1

	#
	# k2htpdtorsvr process on trans server
	#
	echo "------ RUN k2htpdtorsvr process on trans server ------------"
	${DTORSVRBIN} -json "${DTOR_TEST_TRANS_SERVER_JSON_STR}" -d ${DEBUGMODE} &
	K2HTPDTORSVRTRANSPID=$!
	echo "k2htpdtorsvr process on trans server pid = ${K2HTPDTORSVRTRANSPID}"
	sleep 1

	#
	# chmpx for trans slave
	#
	echo "------ RUN chmpx on trans slave side -----------------------"
	chmpx -json "${DTOR_TEST_TRANS_SLAVE_JSON_STR}" -d ${DEBUGMODE} &
	CHMPXSLVTRANSPID=$!
	echo "chmpx on trans slave side pid = ${CHMPXSLVTRANSPID}"
	sleep 5

	#
	# chmpx for server
	#
	echo "------ RUN chmpx on server side ----------------------------"
	chmpx -json "${DTOR_TEST_SERVER_JSON_STR}" -d ${DEBUGMODE} &
	CHMPXSVRPID=$!
	echo "chmpx on server side pid = ${CHMPXSVRPID}"
	sleep 1

	#
	# k2htpdtorsvr process on server
	#
	echo "------ RUN k2htpdtorsvr process on server ------------------"
	${DTORSVRBIN} -json "${DTOR_TEST_SERVER_JSON_STR}" -d ${DEBUGMODE} &
	K2HTPDTORSVRPID=$!
	echo "k2htpdtorsvr process on server pid = ${K2HTPDTORSVRPID}"
	sleep 1

	#
	# chmpx for slave
	#
	echo "------ RUN chmpx on slave side -----------------------------"
	chmpx -json "${DTOR_TEST_SLAVE_JSON_STR}" -d ${DEBUGMODE} &
	CHMPXSLVPID=$!
	echo "chmpx on slave side pid = ${CHMPXSLVPID}"
	sleep 5

	#
	# test client process
	#
	echo "------ RUN & START test client process on slave side -------"
	${DTORCLTBIN} -f ${K2HTPSCRIPTDIR}/dtor_test_archive.data -l ${DTORLIBSO} -p "${DTOR_TEST_SLAVE_JSON_STR}" -c 1
	sleep 10

	##############
	# Stop
	##############
	echo "------ STOP all processes ----------------------------------"
	kill -HUP ${K2HTPDTORSVRPID}
	sleep 1
	kill -HUP ${CHMPXSLVPID}
	sleep 1
	kill -HUP ${CHMPXSVRPID}
	sleep 1
	kill -HUP ${CHMPXSLVTRANSPID}
	sleep 1
	kill -HUP ${K2HTPDTORSVRTRANSPID}
	sleep 1
	kill -HUP ${CHMPXSVRTRANSPID}
	sleep 1
	kill -9 ${K2HTPDTORSVRPID} ${CHMPXSLVPID} ${CHMPXSVRPID} ${CHMPXSLVTRANSPID} ${K2HTPDTORSVRTRANSPID} ${CHMPXSVRTRANSPID} > /dev/null 2>&1

	##############
	# Check
	##############
	echo "------ Check all result by testing -------------------------"
	cmp /tmp/dtor_test_trans_slave_archive.log /tmp/dtor_test_slave_archive.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare dtor_test_trans_slave_archive.log and dtor_test_slave_archive.log)"
		exit 1
	fi

	cmp /tmp/k2hftdtorsvr_trans_archive.log /tmp/k2hftdtorsvr_archive.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare k2hftdtorsvr_trans_archive.log and k2hftdtorsvr_archive.log)"
		exit 1
	fi

	diff /tmp/k2hftdtorsvr_trans_plugin.log /tmp/k2hftdtorsvr_plugin.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare k2hftdtorsvr_trans_plugin.log and k2hftdtorsvr_plugin.log)"
		exit 1
	fi

	DTOR_AR_LOGCNT=`cat -v /tmp/dtor_test_slave_archive.log | grep K2H_ | wc -l`
	SVR_AR_LOGCNT=`cat -v /tmp/k2hftdtorsvr_archive.log | grep K2H_ | wc -l`
	PLUGIN_LOGCNT=`grep -e'DEL_KEY' -e'SET_ALL' -e'REP_VAL' -e'REP_SKEY' -e'OW_VAL' -e'REP_ATTR' -e'REN_KEY' /tmp/k2hftdtorsvr_plugin.log | wc -l`

	if [ $DTOR_AR_LOGCNT -ne $SVR_AR_LOGCNT -o $DTOR_AR_LOGCNT -ne $PLUGIN_LOGCNT ]; then
		echo "RESULT --> FAILED"
		echo "(archive log line count is diffrent)"
		exit 1
	fi

	echo "JSON STRING CONF TEST RESULT --> SUCCEED"
fi

##############################################################
# Start json environment conf
##############################################################
if [ "X${DO_JSON_ENV}" = "Xyes" ]; then
	echo ""
	echo ""
	echo "====== START TEST FOR JSON ENVIRONMENT ====================="
	echo ""

	##############
	# Initialize
	##############
	echo "------ Clear old log ---------------------------------------"
	#
	# Clear logs
	#
	rm -f /tmp/dtor_test_slave_archive.log
	rm -f /tmp/dtor_test_trans_slave_archive.log
	rm -f /tmp/k2hftdtorsvr_archive.log
	rm -f /tmp/k2hftdtorsvr_trans_archive.log
	rm -f /tmp/k2hftdtorsvr_plugin.log
	rm -f /tmp/k2hftdtorsvr_trans_plugin.log
	rm -f /tmp/k2hftdtorsvr.k2h
	rm -f /tmp/k2hftdtorsvr_trans.k2h
	rm -f /tmp/k2htpdtorsvr_np_*

	##############
	# Run
	##############
	#
	# chmpx for trans server
	#
	echo "------ RUN chmpx on trans server side ----------------------"
	CHMJSONCONF="${DTOR_TEST_TRANS_SERVER_JSON_STR}" chmpx -d ${DEBUGMODE} &
	CHMPXSVRTRANSPID=$!
	echo "chmpx on trans server side pid = ${CHMPXSVRTRANSPID}"
	sleep 1

	#
	# k2htpdtorsvr process on trans server
	#
	echo "------ RUN k2htpdtorsvr process on trans server ------------"
	DTORSVRJSONCONF="${DTOR_TEST_TRANS_SERVER_JSON_STR}" ${DTORSVRBIN} -d ${DEBUGMODE} &
	K2HTPDTORSVRTRANSPID=$!
	echo "k2htpdtorsvr process on trans server pid = ${K2HTPDTORSVRTRANSPID}"
	sleep 1

	#
	# chmpx for trans slave
	#
	echo "------ RUN chmpx on trans slave side -----------------------"
	CHMJSONCONF="${DTOR_TEST_TRANS_SLAVE_JSON_STR}" chmpx -d ${DEBUGMODE} &
	CHMPXSLVTRANSPID=$!
	echo "chmpx on trans slave side pid = ${CHMPXSLVTRANSPID}"
	sleep 5

	#
	# chmpx for server
	#
	echo "------ RUN chmpx on server side ----------------------------"
	CHMJSONCONF="${DTOR_TEST_SERVER_JSON_STR}" chmpx -d ${DEBUGMODE} &
	CHMPXSVRPID=$!
	echo "chmpx on server side pid = ${CHMPXSVRPID}"
	sleep 1

	#
	# k2htpdtorsvr process on server
	#
	echo "------ RUN k2htpdtorsvr process on server ------------------"
	DTORSVRJSONCONF="${DTOR_TEST_SERVER_JSON_STR}" ${DTORSVRBIN} -d ${DEBUGMODE} &
	K2HTPDTORSVRPID=$!
	echo "k2htpdtorsvr process on server pid = ${K2HTPDTORSVRPID}"
	sleep 1

	#
	# chmpx for slave
	#
	echo "------ RUN chmpx on slave side -----------------------------"
	CHMJSONCONF="${DTOR_TEST_SLAVE_JSON_STR}" chmpx -d ${DEBUGMODE} &
	CHMPXSLVPID=$!
	echo "chmpx on slave side pid = ${CHMPXSLVPID}"
	sleep 5

	#
	# test client process
	#
	echo "------ RUN & START test client process on slave side -------"
	${DTORCLTBIN} -f ${K2HTPSCRIPTDIR}/dtor_test_archive.data -l ${DTORLIBSO} -p "${DTOR_TEST_SLAVE_JSON_STR}" -c 1
	sleep 10

	##############
	# Stop
	##############
	echo "------ STOP all processes ----------------------------------"
	kill -HUP ${K2HTPDTORSVRPID}
	sleep 1
	kill -HUP ${CHMPXSLVPID}
	sleep 1
	kill -HUP ${CHMPXSVRPID}
	sleep 1
	kill -HUP ${CHMPXSLVTRANSPID}
	sleep 1
	kill -HUP ${K2HTPDTORSVRTRANSPID}
	sleep 1
	kill -HUP ${CHMPXSVRTRANSPID}
	sleep 1
	kill -9 ${K2HTPDTORSVRPID} ${CHMPXSLVPID} ${CHMPXSVRPID} ${CHMPXSLVTRANSPID} ${K2HTPDTORSVRTRANSPID} ${CHMPXSVRTRANSPID} > /dev/null 2>&1

	##############
	# Check
	##############
	echo "------ Check all result by testing -------------------------"
	cmp /tmp/dtor_test_trans_slave_archive.log /tmp/dtor_test_slave_archive.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare dtor_test_trans_slave_archive.log and dtor_test_slave_archive.log)"
		exit 1
	fi

	cmp /tmp/k2hftdtorsvr_trans_archive.log /tmp/k2hftdtorsvr_archive.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare k2hftdtorsvr_trans_archive.log and k2hftdtorsvr_archive.log)"
		exit 1
	fi

	diff /tmp/k2hftdtorsvr_trans_plugin.log /tmp/k2hftdtorsvr_plugin.log > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "RESULT --> FAILED"
		echo "(compare k2hftdtorsvr_trans_plugin.log and k2hftdtorsvr_plugin.log)"
		exit 1
	fi

	DTOR_AR_LOGCNT=`cat -v /tmp/dtor_test_slave_archive.log | grep K2H_ | wc -l`
	SVR_AR_LOGCNT=`cat -v /tmp/k2hftdtorsvr_archive.log | grep K2H_ | wc -l`
	PLUGIN_LOGCNT=`grep -e'DEL_KEY' -e'SET_ALL' -e'REP_VAL' -e'REP_SKEY' -e'OW_VAL' -e'REP_ATTR' -e'REN_KEY' /tmp/k2hftdtorsvr_plugin.log | wc -l`

	if [ $DTOR_AR_LOGCNT -ne $SVR_AR_LOGCNT -o $DTOR_AR_LOGCNT -ne $PLUGIN_LOGCNT ]; then
		echo "RESULT --> FAILED"
		echo "(archive log line count is diffrent)"
		exit 1
	fi

	echo "JSON STRING CONF TEST RESULT --> SUCCEED"
fi

##############################################################
# Finished all test
##############################################################
#
# Clear
#
echo "====== Clear all temporary files ==========================="
rm -f /tmp/dtor_test_slave_archive.log
rm -f /tmp/dtor_test_trans_slave_archive.log
rm -f /tmp/k2hftdtorsvr_archive.log
rm -f /tmp/k2hftdtorsvr_trans_archive.log
rm -f /tmp/k2hftdtorsvr_plugin.log
rm -f /tmp/k2hftdtorsvr_trans_plugin.log
rm -f /tmp/k2htpdtorsvr_plugin.sh
rm -f /tmp/k2hftdtorsvr.k2h
rm -f /tmp/k2hftdtorsvr_trans.k2h
rm -f /tmp/k2htpdtorsvr_np_*

echo ""
echo "====== Finish all =========================================="
exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
