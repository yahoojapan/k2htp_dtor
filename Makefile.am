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
# CREATE:   Thu Feb 26 2015
# REVISION:
#

SUBDIRS=lib src docs tests buildutils
EXTRA_DIST=RELEASE_VERSION @CONFIGURECUSTOM@

CPPCHECK_CMD=	cppcheck
CPPCHECK_OPT=	--quiet \
				--error-exitcode=1 \
				--inline-suppr \
				-j 4 \
				--std=c++03 \
				--xml \
				--enable=warning,style,information,missingInclude
CPPCHECK_IGN=

cppcheck:
	$(CPPCHECK_CMD) $(CPPCHECK_OPT) $(CPPCHECK_IGN) $(SUBDIRS)

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
