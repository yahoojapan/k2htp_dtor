#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# k2htpdtor for K2HASH TRANSACTION PLUGIN.
#
# Copyright 2015 Yahoo! JAPAN corporation.
#
# For the full copyright and license information, please view
# the LICENSE file that was distributed with this source code.
#
# K2HASH TRANSACTION PLUGIN is programable I/F for processing
# transaction data from modifying K2HASH data.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Thu Feb 26 2015
# REVISION:
#

AC_PREREQ([2.63])
AC_INIT(k2htpdtor, m4_esyscmd([tr -d '\n' < $(pwd)/RELEASE_VERSION]))
AM_INIT_AUTOMAKE()
LT_INIT()

AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB(dl, dlopen)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h stdio.h string.h fcntl.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_FORK
AC_CHECK_FUNCS([memset strcasecmp dup2 fdatasync mkdir mkfifo realpath strdup])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES([Makefile
		docs/Makefile
		lib/Makefile
		src/Makefile
		tests/Makefile])

# k2hash
if test "x${CHECK_K2HASH}" != "xno"; then
	PKG_CHECK_MODULES([k2hash], [libk2hash >= 1.0.54], [have_k2hash=yes], [have_k2hash=no])
	if test "x${have_k2hash}" = "xno"; then
		AC_MSG_ERROR([not found k2hash package.])
	fi
fi

# chmpx
if test "x${CHECK_CHMPX}" != "xno"; then
	PKG_CHECK_MODULES([chmpx], [libchmpx >= 1.0.51], [have_chmpx=yes], [have_chmpx=no])
	if test "x${have_chmpx}" = "xno"; then
		AC_MSG_ERROR([not found chmpx package.])
	fi
fi

#fullock
if test "x${CHECK_FULLOCK}" != "xno"; then
	PKG_CHECK_MODULES([fullock], [libfullock >= 1.0.23], [have_fullock=yes], [have_fullock=no])
	if test "x${have_fullock}" = "xno"; then
		AC_MSG_ERROR([not found fullock package.])
	fi
fi

# CFLAGS/CXXFLAGS
CFLAGS="-Wall $CFLAGS"
CXXFLAGS="-Wall $CXXFLAGS"

# LIB_VERSION_INFO for libtool library version info
LIB_VERSION_INFO=`$(pwd)/make_valiables.sh -lib_version_info`
AC_SUBST([LIB_VERSION_INFO])

AC_OUTPUT

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
