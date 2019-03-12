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
 * CREATE:   Thu Feb 26 2015
 * REVISION:
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>

#include <k2hash/k2htransfunc.h>
#include <k2hash/k2hutil.h>
#include <k2hash/k2hdbg.h>
#include <chmpx/chmpx.h>
#include <fullock/fullock.h>

#include "k2htpdtorman.h"
#include "k2htpdtorfile.h"

using namespace std;

// [NOTES] for DEBUG
// 
// If you debug this module, k2hash library and chmpx library, you can set
// K2HDBGMODE and CHMDBGMODE environment as one of SILENT/ERR/WAN/INFO.
// Thus you can get messages from this module and libraries.
// 
//---------------------------------------------------------
// Prototypes
//---------------------------------------------------------
extern "C" {
	bool k2h_trans(k2h_h handle, PBCOM pBinCom);
	const char* k2h_trans_version(void);
	bool k2h_trans_cntl(k2h_h handle, PTRANSOPT pOpt);
}

//---------------------------------------------------------
// Version
//---------------------------------------------------------
// This value is set in k2htpbaseversion.cc file which is made
// by make_rev.sh script.
//
extern char k2htpdtor_commit_hash[];

//---------------------------------------------------------
// Transaction functions
//---------------------------------------------------------
// 
// Transaction function
// 
// This function is called from k2hash library at each one transaction doing.
// You can do anything here for one transaction. If you set transaction file
// path in k2h_trans_cntl function, you can get it's file descriptor by calling
// K2HTransManager::GetArchiveFd() function.
// When you get transaction fd, it is locked automatically by k2hash library.
// Thus it is safe for multi thread.
// 
// This function gets BCOM structure pointer, this structure(union) is specified
// in k2hcommand.h. For example, v1.0.12 k2hash defined following:
// 
//		typedef struct serialize_command{
//			char	szCommand[SCOM_COMSTR_LENGTH];
//			long	type;
//			size_t	key_length;
//			size_t	val_length;
//			size_t	skey_length;
//			size_t	exdata_length;
//			off_t	key_pos;
//			off_t	val_pos;
//			off_t	skey_pos;
//			off_t	exdata_pos;
//		}K2HASH_ATTR_PACKED SCOM, *PSCOM;
// 
//		typedef union binary_command{
//			SCOM			scom;
//			unsigned char	byData[sizeof(SCOM)];
//		}K2HASH_ATTR_PACKED BCOM, *PBCOM;
//
bool k2h_trans(k2h_h handle, PBCOM pBinCom)
{
	if(!pBinCom){
		return false;
	}
	K2HtpDtorManager*	pDtorMan = K2HtpDtorManager::Get();

	// [NOTE]
	// IsExcept processes IsExceptKey and IsExceptType.
	// 
	if(pDtorMan->IsExcept(handle, &(pBinCom->byData[pBinCom->scom.key_pos]), pBinCom->scom.key_length, pBinCom->scom.type)){
		//MSG_K2HPRN("K2HTPDTOR transaction type in pbcom is exception(key or type), so skip this transaction.");
		return true;
	}

	// Get msgid
	chmpx_h	chmpxhandle	= CHM_INVALID_CHMPXHANDLE;
	msgid_t	msgid		= CHM_INVALID_MSGID;
	bool	isbroadcast	= false;
	if(!pDtorMan->GetChmpxMsgId(handle, chmpxhandle, msgid, isbroadcast) || CHM_INVALID_CHMPXHANDLE == chmpxhandle || CHM_INVALID_MSGID == msgid){
		ERR_K2HPRN("K2HTPDTOR could not get msgid for k2hash handle(0x%016" PRIx64 ").", handle);
		return false;
	}

	// make send buffer struct.
	CHMBIN			chmbin;
	chmbin.byptr	= pBinCom->byData;
	chmbin.length	= scom_total_length(pBinCom->scom);

	// make hash code
	chmhash_t	hash = make_chmbin_hash(&chmbin);

	// put output file
	int	outputfd;
	if(DTOR_INVALID_HANDLE != (outputfd = pDtorMan->GetOutputFd(handle, true))){
		// seek
		if(-1 == lseek(outputfd, 0, SEEK_END)){
			ERR_K2HPRN("Could not seek file to end by errno(%d), but continue...", errno);
		}else{
			// write
			if(!k2htpdtor_write(outputfd, chmbin.byptr, chmbin.length)){
				ERR_K2HPRN("Failed to write command to output file, but continue...");
			}else{
				fdatasync(outputfd);
			}
		}
		// unlock
		int	result;
		if(0 != (result = fullock_rwlock_unlock(outputfd, 0, 1))){
			ERR_K2HPRN("Could not unlock for output file by errno(%d), but continue...", result);
		}
	}

	// send
	//
	// [TODO]
	// The Routing mode is always false, it maybe correct.
	// But if you need to control routing mode, you have to read the mode in configuration file and keep
	// it in dtor mapping data.
	//
	long	preceivercnt = 0L;
	if(isbroadcast){
		if(!chmpx_msg_broadcast(chmpxhandle, msgid, chmbin.byptr, chmbin.length, hash, &preceivercnt)){
			ERR_K2HPRN("K2HTPDTOR could not broadcast data to msgid(0x%016" PRIx64 "), chmpx handle(0x%016" PRIx64 "), k2hash handle(0x%016" PRIx64 ").", msgid, chmpxhandle, handle);
			return false;
		}
	}else{
		if(!chmpx_msg_send(chmpxhandle, msgid, chmbin.byptr, chmbin.length, hash, &preceivercnt, true)){
			ERR_K2HPRN("K2HTPDTOR could not send data to msgid(0x%016" PRIx64 "), chmpx handle(0x%016" PRIx64 "), k2hash handle(0x%016" PRIx64 ").", msgid, chmpxhandle, handle);
			return false;
		}
	}

	if(0L == preceivercnt){
		ERR_K2HPRN("K2HTPDTOR there is no receiver for msgid(0x%016" PRIx64 "), chmpx handle(0x%016" PRIx64 "), k2hash handle(0x%016" PRIx64 ").", msgid, chmpxhandle, handle);
		return false;
	}
	return true;
}

// 
// Transaction Version function for this plugin
// 
// This function is called from k2hash library when getting transaction plugin version.
// For example, k2hlinetool can display transaction plugin version by info and trans
// command.
//
const char* k2h_trans_version(void)
{
	// Following codes returns this plugin package version number and Github commit hash.
	//
	static string	trans_version("");
	static bool		is_set = false;

	if(!is_set){
		trans_version	=  "k2htpdtor K2HTP V";
		trans_version	+= VERSION;
		trans_version	+= "(";
		trans_version	+= k2htpdtor_commit_hash;
		trans_version	+= ")";
		is_set = true;
	}
	return trans_version.c_str();
}

// 
// Transaction Control function
// 
// This function is called from k2hash library when transaction is enable/disable.
// You can control disable/enable for transaction by setting TRANSOPT structure.
// And if you set enable, you can specify transaction file path and transaction
// queue prefix.
// 
// This function gets TRANSOPT structure pointer which has below member.
//		pOpt->szFilePath[MAX_TRANSACTION_FILEPATH]
//		pOpt->isEnable
//		pOpt->byTransPrefix[MAX_TRANSACTION_PREFIX]
//		pOpt->PrefixLength
//		pOpt->byTransParam[MAX_TRANSACTION_PARAM];
//		pOpt->ParamLength;
// You must set each member value before this function returns.
// 
// isEnable			This value means transaction going to disable or enable.
// 					This function can know it from this member value.
// 					If you set transaction to disable despite this value is true,
//					you can set false to this value before returning.
// szFilePath		When isEnable is only true, this value means transaction file
//					path which is specified at called K2HShm::Transaction().
//					If you need to modify(or change) this value, you can set it
//					in this function.(over write this value)
//					If this value is not NULL, in the k2h_trans function you can get
//					file pointer to this file path by K2HTransManager::GetArchiveFd()
//					function. Then this archive fd is locked and opened, so you
//					can only push data to it.
//					If isEnable is false, you do not have to care this value.
// byTransPrefix	When isEnable is only true, this value means the queue name
//					prefix for transaction in k2hash. This value is set at calling
//					K2HShm::Transaction(). And the value is set in this buffer at
//					calling this handler. If you want to modify(or change) prefix for
//					transaction queue name, you can set(over write) this value in this
//					function. If you use default queue name, you do not have to set this
//					value. If isEnable is false, you do not have to care this value.
// PrefixLength		If you set byTransPrefix, you must set this value for byTransPrefix
//					length.
// byTransParam		When isEnable is only true, this value means the custom parameter
//					which the transaction control function is called with.
// ParamLength		If you set byTransParam, you must set this value for byTransParam
//					length
//
bool k2h_trans_cntl(k2h_h handle, PTRANSOPT pOpt)
{
	if(!pOpt){
		return false;
	}
	if(pOpt->isEnable){
		if('\0' != pOpt->szFilePath[0]){
			WAN_K2HPRN("K2HTPDTOR does not use transaction file(%s), so clear this.", pOpt->szFilePath);
			memset(pOpt->szFilePath, 0, MAX_TRANSACTION_FILEPATH);
		}

		if(0 < pOpt->PrefixLength){
			MSG_K2HPRN("K2HTPDTOR change transaction queue prefix.");
		}

		if(0 == pOpt->ParamLength){
			ERR_K2HPRN("K2HTPDTOR needs parameter which is CHMPX group name.");
			return false;
		}

		// k2hash handle -> k2htpdtor configuration file( or including CHMPX configuration file path )
		if(!K2HtpDtorManager::Get()->Add(handle, reinterpret_cast<const char*>(pOpt->byTransParam))){
			ERR_K2HPRN("K2HTPDTOR could not set k2hash handle(0x%016" PRIx64 ") and chmpx conf file(%s).", handle, reinterpret_cast<const char*>(pOpt->byTransParam));
			return false;
		}

	}else{
		// remove k2hash handle mapping(and detach chmpx)
		if(!K2HtpDtorManager::Get()->Remove(handle)){
			ERR_K2HPRN("K2HTPDTOR could not remove k2hash handle(0x%016" PRIx64 ").", handle);
			return false;
		}
	}
	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
