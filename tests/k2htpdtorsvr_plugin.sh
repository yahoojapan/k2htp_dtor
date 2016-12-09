#!/bin/sh
#
# The BASE templates for K2HASH TRANSACTION PLUGIN.
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
# CREATE:   Thu Nov 05 2015
# REVISION:
#

#
# This script is test k2htpdtorsvr plugin.
#
# Usage: k2htpdtorsvr_plugin.sh <log file path>
#

PRGNAME=`basename $0`

if [ $# -ne 1 ]; then
	echo "${PRGNAME}: Error - parameter is not found."
	exit 1
fi
FILEPATH=$1

touch $FILEPATH

#
# Loop
#
while true; do
	read TRANSACTIONDATA
	echo $TRANSACTIONDATA >> $FILEPATH
done

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
