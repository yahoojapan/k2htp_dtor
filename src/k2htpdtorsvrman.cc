/*
 * k2htpdtor for K2HASH TRANSACTION PLUGIN.
 *
 * Copyright 2015 Yahoo Japan Corporation.
 *
 * K2HASH TRANSACTION PLUGIN is programable I/F for processing
 * transaction data from modifying K2HASH data.
 *
 * For the full copyright and license information, please view
 * the license file that was distributed with this source code.
 *
 * AUTHOR:   Takeshi Nakatani
 * CREATE:   Thu Feb 26 2015
 * REVISION:
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/inotify.h>

#include <k2hash/k2htransfunc.h>
#include <k2hash/k2hutil.h>
#include <k2hash/k2hdbg.h>
#include <chmpx/chmcntrl.h>

#include <string>

#include "k2htpdtorsvrman.h"

using namespace std;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	K2HTPDTORSVR_K2HTPDTOR_LIB					"libk2htpdtor.so.1"

#define	TOPLUGIN_SET_BOTH							"SET_ALL "
#define	TOPLUGIN_REP_VALUE							"REP_VAL "
#define	TOPLUGIN_REP_SUBKEY							"REP_SKEY "
#define	TOPLUGIN_DEL_KEY							"DEL_KEY "
#define	TOPLUGIN_OW_VALUE							"OW_VAL "
#define	TOPLUGIN_REPLACE_ATTRS						"REP_ATTRS "
#define	TOPLUGIN_RENAME								"REN_KEY "

#define	TOPLUGIN_FORM_1PARAM						"1 %zu "
#define	TOPLUGIN_FORM_2PARAM						"2 %zu %zu "
#define	TOPLUGIN_FORM_3PARAM						"3 %zu %zu %zu "
#define	TOPLUGIN_FORM_4PARAM						"4 %zu %zu %zu %zu "

//---------------------------------------------------------
// local data
//---------------------------------------------------------
static const unsigned char	byTerminate[] = {0x0A};		// \n
static const unsigned char	bySeparater[] = {0x20};		// \s

//---------------------------------------------------------
// K2HtpSvrManager : Class variables
//---------------------------------------------------------
volatile bool	K2HtpSvrManager::is_doing_loop = false;

//---------------------------------------------------------
// K2HtpSvrManager : Class Methods
//---------------------------------------------------------
void K2HtpSvrManager::SigHuphandler(int signum)
{
	if(SIGHUP == signum){
		K2HtpSvrManager::is_doing_loop = false;
	}
}

//---------------------------------------------------------
// K2HtpSvrManager : Methods
//---------------------------------------------------------
K2HtpSvrManager::K2HtpSvrManager() : pk2hash(NULL), pplugin(NULL), pfilefd(NULL), is_load_dtor(false)
{
}

K2HtpSvrManager::~K2HtpSvrManager()
{
	Clean();
}

bool K2HtpSvrManager::Clean(void)
{
	bool	result = true;

	// output file
	if(pfilefd){
		K2H_Delete(pfilefd);
	}

	// plugin
	if(pplugin){
		if(!pplugin->Clean()){
			ERR_K2HPRN("Failed to cleanup plugin, but continue...");
			result = false;
		}
		K2H_Delete(pplugin);
	}

	// k2hash & transaction
	if(pk2hash){
		if(is_load_dtor){
			if(!pk2hash->DisableTransaction()){
				ERR_K2HPRN("Failed to stop transaction, but continue...");
				result = false;
			}
		}

		if(!pk2hash->Detach()){
			ERR_K2HPRN("Failed to detach k2hash, but continue...");
			result = false;
		}
		K2H_Delete(pk2hash);

		if(is_load_dtor){
			if(!K2HTransDynLib::get()->Unload()){
				ERR_K2HPRN("Could not unload transaction plugin, but continue...");
				result = false;
			}
			if(!K2HShm::UnsetTransThreadPool()){
				ERR_K2HPRN("Could not unset thread pool count, but continue...");
				result = false;
			}
		}
	}
	return result;
}

bool K2HtpSvrManager::Initialize(PK2HTPDTORSVRINFO pInfo)
{
	if(!pInfo){
		ERR_K2HPRN("parameters are wrong.");
		return false;
	}

	// output file
	if(!pInfo->output_file.empty()){
		pfilefd = new K2htpSvrFd;
		if(!pfilefd->Initialize(pInfo->output_file.c_str())){
			ERR_K2HPRN("could not create output file(%s) object.", pInfo->output_file.c_str());
			Clean();
			return false;
		}
	}

	// plugin
	if(!pInfo->plugin_param.empty()){
		pplugin = new K2htpSvrPlugin;
		if(!pplugin->Initialize(pInfo->plugin_param.c_str())){
			ERR_K2HPRN("could not create plugin(%s) object.", pInfo->plugin_param.c_str());
			Clean();
			return false;
		}
	}

	// k2hash & transaction
	if(pInfo->is_k2hash){
		if(pInfo->k2hash_init && !pInfo->k2hash_file.empty()){
			// need to initialize k2hash, so remove file.
			if(0 != unlink(pInfo->k2hash_file.c_str())){
				ERR_K2HPRN("could not remove file(%s) by errno(%d), but continue...", pInfo->k2hash_file.c_str(), errno);
			}
		}

		// load transaction plugin
		if(!pInfo->trans_ini_file.empty()){
			// load k2htpdtor transaction plugin
			if(!K2HTransDynLib::get()->Load(K2HTPDTORSVR_K2HTPDTOR_LIB)){
				ERR_K2HPRN("Could not load %s transaction plugin.", K2HTPDTORSVR_K2HTPDTOR_LIB);
				Clean();
				return false;
			}
			is_load_dtor = true;

			// set thread pool count
			if(K2HTransManager::DEFAULT_THREAD_POOL != pInfo->trans_dtor_threads){
				if(!K2HShm::SetTransThreadPool(pInfo->trans_dtor_threads)){
					ERR_K2HPRN("Could not set thread pool count(%d), but continue...", pInfo->trans_dtor_threads);
				}
			}
		}

		// attach k2hash
		pk2hash = new K2HShm;
		if(!pk2hash->Attach(pInfo->k2hash_file.c_str(), false, true, pInfo->k2hash_type_tmp, pInfo->k2hash_fullmap, pInfo->k2hash_mask_bitcnt, pInfo->k2hash_cmask_bitcnt, pInfo->k2hash_max_element_cnt, pInfo->k2hash_pagesize)){
			ERR_K2HPRN("Failed to attach k2hash %s(%s).", (pInfo->k2hash_type_mem ? "memory" : pInfo->k2hash_type_tmp ? "temporary file" : "file"), (pInfo->k2hash_file.empty() ? "no file" : pInfo->k2hash_file.c_str()));
			Clean();
			return false;
		}

		// enable transaction plugin
		if(!pInfo->trans_ini_file.empty()){
			if(!pk2hash->EnableTransaction(pInfo->k2hash_file.c_str(), NULL, 0L, reinterpret_cast<const unsigned char*>(pInfo->trans_ini_file.c_str()), pInfo->trans_ini_file.length() + 1)){
				ERR_K2HPRN("Failed to enable transaction plugin(ini:%s) for k2hash %s(%s).", pInfo->trans_ini_file.c_str(), (pInfo->k2hash_type_mem ? "memory" : pInfo->k2hash_type_tmp ? "temporary file" : "file"), (pInfo->k2hash_file.empty() ? "no file" : pInfo->k2hash_file.c_str()));
				Clean();
				return false;
			}
		}
	}

	// Join chmpx server side
	if(!chmpxobj.InitializeOnServer(pInfo->conffile.c_str(), true)){		// auto rejoin
		ERR_K2HPRN("Could not join chmpx by configration file %s.", pInfo->conffile.c_str());
		Clean();
		return false;
	}

	// set exit signal
	struct sigaction	sa;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGHUP);
	sa.sa_flags		= 0;
	sa.sa_handler	= K2HtpSvrManager::SigHuphandler;
	if(0 > sigaction(SIGHUP, &sa, NULL)){
		ERR_K2HPRN("Could not set signal HUP handler. errno = %d", errno);
		Clean();
		return false;
	}
	return true;
}

bool K2HtpSvrManager::Processing(void)
{
	// do loop
	for(K2HtpSvrManager::is_doing_loop = true; K2HtpSvrManager::is_doing_loop; ){
		PCOMPKT			pComPkt	= NULL;
		unsigned char*	pbody	= NULL;
		size_t			length	= 0L;

		// receive message
		if(!chmpxobj.Receive(&pComPkt, &pbody, &length, 1000, true)){		// timeout 1s, auto rejoin
			ERR_K2HPRN("Something error occured at waiting message.");
			continue;
		}
		if(!pComPkt){
			MSG_K2HPRN("Timeouted 1s for waiting message, so continue to waiting.");
			continue;
		}
		if(!pbody){
			ERR_K2HPRN("NULL body data recieved.");
			CHM_Free(pComPkt);
			continue;
		}
		MSG_K2HPRN("Succeed to receive message.");

		// check received data
		PBCOM	pBinCom = reinterpret_cast<PBCOM>(pbody);
		if(length != scom_total_length(pBinCom->scom)){
			ERR_K2HPRN("Length(%zu) of recieved message body is wrong(should be %zu).", length, scom_total_length(pBinCom->scom));
			CHM_Free(pComPkt);
			CHM_Free(pbody);
			continue;
		}

		// check received data type
		if(pBinCom->scom.type < SCOM_TYPE_MIN || SCOM_TYPE_MAX < pBinCom->scom.type){
			ERR_K2HPRN("BCom data type(%ld: %s) is unknown.", pBinCom->scom.type, pBinCom->scom.szCommand);
			CHM_Free(pComPkt);
			CHM_Free(pbody);
			continue;
		}

		// check data and set pointers
		unsigned char*	byKey	= 0L < pBinCom->scom.key_length ?	static_cast<unsigned char*>(&pBinCom->byData[pBinCom->scom.key_pos])	: NULL;
		unsigned char*	byVal	= 0L < pBinCom->scom.val_length ?	static_cast<unsigned char*>(&pBinCom->byData[pBinCom->scom.val_pos])	: NULL;
		unsigned char*	bySKey	= 0L < pBinCom->scom.skey_length ?	static_cast<unsigned char*>(&pBinCom->byData[pBinCom->scom.skey_pos])	: NULL;
		unsigned char*	byAttr	= 0L < pBinCom->scom.attr_length ?	static_cast<unsigned char*>(&pBinCom->byData[pBinCom->scom.attrs_pos])	: NULL;
		unsigned char*	byExdata= 0L < pBinCom->scom.exdata_length ?static_cast<unsigned char*>(&pBinCom->byData[pBinCom->scom.exdata_pos])	: NULL;
		if(!byKey && !byVal && !bySKey && !byAttr && !byExdata){
			ERR_K2HPRN("BCom data is something wrong.");
			CHM_Free(pComPkt);
			CHM_Free(pbody);
			continue;
		}

		// do processing...
		if(pfilefd){
			// output file(write recieved data without changing)
			if(!pfilefd->Write(pbody, length)){
				ERR_K2HPRN("Failed to write to output file.");
			}
		}
		if(pplugin){
			// plugin
			char	szPrefix[256];
			bool	result = true;
			if(SCOM_SET_ALL == pBinCom->scom.type){
				//
				// "SET_ALL 4 (key length) (value length) (subkey length) (attr length) (key) (value) (subkeys) (attrs)"
				//
				sprintf(szPrefix, TOPLUGIN_SET_BOTH TOPLUGIN_FORM_4PARAM, pBinCom->scom.key_length, pBinCom->scom.val_length, pBinCom->scom.skey_length, pBinCom->scom.attr_length);
				if(	!pplugin->Write(reinterpret_cast<unsigned char*>(szPrefix), strlen(szPrefix))			||
					(0 != pBinCom->scom.key_length	&& !pplugin->Write(byKey, pBinCom->scom.key_length))	||
					!pplugin->Write(bySeparater, sizeof(bySeparater))										||
					(0 != pBinCom->scom.val_length	&& !pplugin->Write(byVal, pBinCom->scom.val_length))	||
					!pplugin->Write(bySeparater, sizeof(bySeparater))										||
					(0 != pBinCom->scom.skey_length	&& !pplugin->Write(bySKey, pBinCom->scom.skey_length))	||
					!pplugin->Write(bySeparater, sizeof(bySeparater))										||
					(0 != pBinCom->scom.attr_length	&& !pplugin->Write(byAttr, pBinCom->scom.attr_length))	||
					!pplugin->Write(byTerminate, sizeof(byTerminate))										)
				{
					result = false;
				}

			}else if(SCOM_REPLACE_VAL == pBinCom->scom.type){
				//
				// "REP_VAL 2 (key length) (value length) (key) (value)"
				//
				sprintf(szPrefix, TOPLUGIN_REP_VALUE TOPLUGIN_FORM_2PARAM, pBinCom->scom.key_length, pBinCom->scom.val_length);
				if(	!pplugin->Write(reinterpret_cast<unsigned char*>(szPrefix), strlen(szPrefix))			||
					(0 != pBinCom->scom.key_length	&& !pplugin->Write(byKey, pBinCom->scom.key_length))	||
					!pplugin->Write(bySeparater, sizeof(bySeparater))										||
					(0 != pBinCom->scom.val_length	&& !pplugin->Write(byVal, pBinCom->scom.val_length))	||
					!pplugin->Write(byTerminate, sizeof(byTerminate))										)
				{
					result = false;
				}

			}else if(SCOM_REPLACE_SKEY == pBinCom->scom.type){
				//
				// "REP_SKEY 2 (key length) (subkey length) (key) (subkeys)"
				//
				sprintf(szPrefix, TOPLUGIN_REP_SUBKEY TOPLUGIN_FORM_2PARAM, pBinCom->scom.key_length, pBinCom->scom.skey_length);
				if(	!pplugin->Write(reinterpret_cast<unsigned char*>(szPrefix), strlen(szPrefix))			||
					(0 != pBinCom->scom.key_length	&& !pplugin->Write(byKey, pBinCom->scom.key_length))	||
					!pplugin->Write(bySeparater, sizeof(bySeparater))										||
					(0 != pBinCom->scom.skey_length	&& !pplugin->Write(bySKey, pBinCom->scom.skey_length))	||
					!pplugin->Write(byTerminate, sizeof(byTerminate))										)
				{
					result = false;
				}

			}else if(SCOM_REPLACE_ATTRS == pBinCom->scom.type){
				//
				// "REP_ATTRS 2 (key length) (attr length) (key) (attrs)"
				//
				sprintf(szPrefix, TOPLUGIN_REPLACE_ATTRS TOPLUGIN_FORM_2PARAM, pBinCom->scom.key_length, pBinCom->scom.attr_length);
				if(	!pplugin->Write(reinterpret_cast<unsigned char*>(szPrefix), strlen(szPrefix))			||
					(0 != pBinCom->scom.key_length	&& !pplugin->Write(byKey, pBinCom->scom.key_length))	||
					!pplugin->Write(bySeparater, sizeof(bySeparater))										||
					(0 != pBinCom->scom.attr_length	&& !pplugin->Write(byAttr, pBinCom->scom.attr_length))	||
					!pplugin->Write(byTerminate, sizeof(byTerminate))										)
				{
					result = false;
				}

			}else if(SCOM_DEL_KEY == pBinCom->scom.type){
				//
				// "DEL_KEY 1 (key length) (key)"
				//
				sprintf(szPrefix, TOPLUGIN_DEL_KEY TOPLUGIN_FORM_1PARAM, pBinCom->scom.key_length);
				if(	!pplugin->Write(reinterpret_cast<unsigned char*>(szPrefix), strlen(szPrefix))			||
					(0 != pBinCom->scom.key_length	&& !pplugin->Write(byKey, pBinCom->scom.key_length))	||
					!pplugin->Write(byTerminate, sizeof(byTerminate))										)
				{
					result = false;
				}

			}else if(SCOM_OW_VAL == pBinCom->scom.type){
				//
				// "OW_VAL 3 (key length) (value length) (offset position length) (key) (value) (offset position=decimal string)"
				//
				if(!byExdata || 0UL == pBinCom->scom.exdata_length){
					result = false;
				}else{
					// make offset string
					OWVAL_EXDATA	exdata;
					char			szoffset[64];
					size_t			offset_len;
					memcpy(exdata.byData, byExdata, pBinCom->scom.exdata_length);
					sprintf(szoffset, "%zd", exdata.valoffset);
					offset_len = strlen(szoffset);

					sprintf(szPrefix, TOPLUGIN_OW_VALUE TOPLUGIN_FORM_3PARAM, pBinCom->scom.key_length, pBinCom->scom.val_length, offset_len);
					if(	!pplugin->Write(reinterpret_cast<unsigned char*>(szPrefix), strlen(szPrefix))		||
						(0 != pBinCom->scom.key_length	&& !pplugin->Write(byKey, pBinCom->scom.key_length))||
						!pplugin->Write(bySeparater, sizeof(bySeparater))									||
						(0 != pBinCom->scom.val_length	&& !pplugin->Write(byVal, pBinCom->scom.val_length))||
						!pplugin->Write(bySeparater, sizeof(bySeparater))									||
						!pplugin->Write(reinterpret_cast<unsigned char*>(&szoffset[0]), offset_len)			||
						!pplugin->Write(byTerminate, sizeof(byTerminate))									)
					{
						result = false;
					}
				}

			}else{	// SCOM_RENAME == pBinCom->scom.type
				//
				// "REN_KEY 3 (key length) (new key length) (attr length) (key) (new key) (attrs)"
				//
				sprintf(szPrefix, TOPLUGIN_RENAME TOPLUGIN_FORM_3PARAM, pBinCom->scom.key_length, pBinCom->scom.exdata_length, pBinCom->scom.attr_length);
				if(	!pplugin->Write(reinterpret_cast<unsigned char*>(szPrefix), strlen(szPrefix))					||
					(0 != pBinCom->scom.key_length	&& !pplugin->Write(byKey, pBinCom->scom.key_length))			||
					!pplugin->Write(bySeparater, sizeof(bySeparater))												||
					(0 != pBinCom->scom.exdata_length	&& !pplugin->Write(byExdata, pBinCom->scom.exdata_length))	||
					!pplugin->Write(bySeparater, sizeof(bySeparater))												||
					(0 != pBinCom->scom.attr_length	&& !pplugin->Write(byAttr, pBinCom->scom.attr_length))			||
					!pplugin->Write(byTerminate, sizeof(byTerminate))												)
				{
					result = false;
				}
			}
			if(!result){
				ERR_K2HPRN("Failed to write to plugin.");
			}
		}

		if(pk2hash){
			// k2hash & transaction
			bool	result;
			if(SCOM_SET_ALL == pBinCom->scom.type){
				K2HSubKeys	SubKeys(bySKey, pBinCom->scom.skey_length);
				K2HAttrs	Attrs(byAttr, pBinCom->scom.attr_length);
				result = pk2hash->Set(byKey, pBinCom->scom.key_length, byVal, pBinCom->scom.val_length, &SubKeys, false, &Attrs);

			}else if(SCOM_REPLACE_VAL == pBinCom->scom.type){
				result = pk2hash->ReplaceValue(byKey, pBinCom->scom.key_length, byVal, pBinCom->scom.val_length);

			}else if(SCOM_REPLACE_SKEY == pBinCom->scom.type){
				result = pk2hash->ReplaceSubkeys(byKey, pBinCom->scom.key_length, bySKey, pBinCom->scom.skey_length);

			}else if(SCOM_REPLACE_ATTRS == pBinCom->scom.type){
				result = pk2hash->ReplaceAttrs(byKey, pBinCom->scom.key_length, byAttr, pBinCom->scom.attr_length);

			}else if(SCOM_DEL_KEY == pBinCom->scom.type){
				result = pk2hash->Remove(byKey, pBinCom->scom.key_length, false);

			}else if(SCOM_OW_VAL == pBinCom->scom.type){
				if(!byExdata || 0UL == pBinCom->scom.exdata_length){
					result = false;
				}else{
					K2HDAccess		daccess(pk2hash, K2HDAccess::WRITE_ACCESS);
					OWVAL_EXDATA	exdata;

					memcpy(exdata.byData, byExdata, pBinCom->scom.exdata_length);
					if(	!daccess.Open(byKey, pBinCom->scom.key_length) ||
						!daccess.SetWriteOffset(exdata.valoffset) ||
						!daccess.Write(byVal, pBinCom->scom.val_length) )
					{
						result = false;
					}else{
						result = true;
					}
				}

			}else{	// SCOM_RENAME == pBinCom->scom.type
				result = pk2hash->Rename(byKey, pBinCom->scom.key_length, byExdata, pBinCom->scom.exdata_length, byAttr, pBinCom->scom.attr_length);

			}
			if(!result){
				ERR_K2HPRN("Failed to write to k2hash.");
			}
		}

		CHM_Free(pComPkt);
		CHM_Free(pbody);
	}
	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
