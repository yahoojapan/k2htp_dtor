/*
 * k2htpdtor for K2HASH TRANSACTION PLUGIN.
 *
 * Copyright 2015 Yahoo Japan Corporation.
 *
 * K2HASH TRANSACTION PLUGIN is programmable I/F for processing
 * transaction data from modifying K2HASH data.
 *
 * For the full copyright and license information, please view
 * the license file that was distributed with this source code.
 *
 * AUTHOR:   Takeshi Nakatani
 * CREATE:   Wed Nov 04 2015
 * REVISION:
 *
 */

#ifndef	K2HTPDTORSVRMAN_H
#define	K2HTPDTORSVRMAN_H

#include <k2hash/k2hshm.h>
#include <chmpx/chmcntrl.h>

#include "k2htpdtorsvrplugin.h"
#include "k2htpdtorsvrfd.h"
#include "k2htpdtorsvrparser.h"

//---------------------------------------------------------
// K2HtpSvrManager Class
//---------------------------------------------------------
class K2HtpSvrManager
{
	protected:
		static volatile bool	is_doing_loop;

		ChmCntrl				chmpxobj;
		K2HShm*					pk2hash;
		K2htpSvrPlugin*			pplugin;
		K2htpSvrFd*				pfilefd;
		bool					is_load_dtor;

	protected:
		static void SigHuphandler(int signum);

	public:
		K2HtpSvrManager();
		virtual ~K2HtpSvrManager();

		bool Initialize(PK2HTPDTORSVRINFO pInfo);
		bool Clean(void);
		bool Processing(void);
};

#endif	// K2HTPDTORSVRMAN_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
