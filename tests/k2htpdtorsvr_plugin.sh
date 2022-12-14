#!/bin/sh
#
# The BASE templates for K2HASH TRANSACTION PLUGIN.
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
# CREATE:   Thu Nov 05 2015
# REVISION:
#

#==============================================================
# This script is test k2htpdtorsvr plugin.
#
# Usage: k2htpdtorsvr_plugin.sh <log file path>
#==============================================================

#--------------------------------------------------------------
# Common Variables
#--------------------------------------------------------------
#PRGNAME=$(basename "${0}")
SCRIPTDIR=$(dirname "${0}")
SCRIPTDIR=$(cd "${SCRIPTDIR}" || exit 1; pwd)

#--------------------------------------------------------------
# Main
#--------------------------------------------------------------
if [ $# -ne 1 ] || [ -z "$1" ]; then
	echo "[ERROR] Parameter is wrong, need to specify <log file path>." 1>&2
	exit 1
fi
FILEPATH="$1"

#
# Update / Create file
#
if ! touch "${FILEPATH}"; then
	echo "[ERROR] Could not update(create) ${FILEPATH} file." 1>&2
	exit 1
fi

#
# Loop
#
echo "Input string to transfer to ${FILEPATH}" 1>&2

IS_LOOP=0
while [ "${IS_LOOP}" -eq 0 ]; do
	read -r TRANSACTIONDATA
	echo "${TRANSACTIONDATA}" >> "${FILEPATH}"
done

exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
