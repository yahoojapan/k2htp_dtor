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
 * CREATE:   Tue Mar 3 2015
 * REVISION:
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <k2hash/k2htransfunc.h>
#include <chmpx/chmpx.h>
#include <chmpx/chmcntrl.h>
#include <chmpx/chmdbg.h>

#include <string>
#include <iostream>

using namespace std;

//---------------------------------------------------------
// Variable
//---------------------------------------------------------
static volatile bool	is_doing_loop = true;

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
//	[Usage]
//	prgname -f <chmpx configration file>
//
static void PrintUsage(char* argv)
{
	string	prgname = argv ? basename(argv) : "k2htpdtorserver";

	cout << "[Usage]" << endl;
	cout << prgname << " -f <chmpx configration file>" << endl;
	cout << endl;
	cout << "-f <chmpx configration file>  specified configration file for chmpx" << endl;
	cout << endl;
}

static bool CheckRegularFile(const char* pfile)
{
	if(!pfile){
		return false;
	}
	struct stat	st;
	if(0 != stat(pfile, &st)){
		ERR_CHMPRN("File %s does not exist.", pfile);
		return false;
	}
	if(!S_ISREG(st.st_mode)){
		ERR_CHMPRN("File %s is not regular file.", pfile);
		return false;
	}
	return true;
}

static bool ParamParser(int argc, char** argv, string& conffile)
{
	conffile.erase();

	for(int cnt = 1; cnt < argc; cnt++){
		if(0 == strcmp(argv[cnt], "-f")){
			if(argc <= cnt + 1){
				ERR_CHMPRN("Option %s needs parameter.", argv[cnt]);
				return false;
			}
			if(!conffile.empty()){
				ERR_CHMPRN("Option %s already set.", argv[cnt]);
				return false;
			}
			if(!CheckRegularFile(argv[++cnt])){
				ERR_CHMPRN("%s file does not regular file.", argv[cnt]);
				return false;
			}
			conffile = argv[cnt];

		}else{
			ERR_CHMPRN("Unknown option %s.", argv[cnt]);
			return false;
		}
	}
	return true;
}

static char* GetPrintableString(const unsigned char* byData, size_t length)
{
	if(!byData || 0 == length){
		return NULL;
	}
	char*	result;
	if(NULL == (result = reinterpret_cast<char*>(calloc(length + 1, sizeof(char))))){
		ERR_CHMPRN("Could not allocate memory.");
		return NULL;
	}
	for(size_t pos = 0; pos < length; ++pos){
		result[pos] = isprint(byData[pos]) ? byData[pos] : (0x00 != byData[pos] ? 0xFF : (pos + 1 < length ? ' ' : 0x00));
	}
	return result;
}

//---------------------------------------------------------
// Signal handler
//---------------------------------------------------------
static void SigHuphandler(int signum)
{
	if(SIGHUP == signum){
		is_doing_loop = false;
	}
}

//---------------------------------------------------------
// Main
//---------------------------------------------------------
int main(int argc, char** argv)
{
	string	conffile;

	SetChmDbgMode(CHMDBG_SILENT);

	// Parse parameter
	if(!ParamParser(argc, argv, conffile) || conffile.empty()){
		PrintUsage(argv[0]);
		exit(EXIT_FAILURE);
	}

	// Join chmpx
	ChmCntrl	ChmpxObj;
	if(!ChmpxObj.InitializeOnServer(conffile.c_str(), true)){		// auto rejoin
		ERR_CHMPRN("Could not join chmpx by configration file %s.", conffile.c_str());
		exit(EXIT_FAILURE);
	}

	// set exit signal
	struct sigaction	sa;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGHUP);
	sa.sa_flags		= 0;
	sa.sa_handler	= SigHuphandler;
	if(0 > sigaction(SIGHUP, &sa, NULL)){
		ERR_CHMPRN("Could not set signal HUP handler. errno = %d", errno);
		exit(EXIT_FAILURE);
	}

	// Recieve and print
	while(is_doing_loop){
		PCOMPKT			pComPkt	= NULL;
		unsigned char*	pbody	= NULL;
		size_t			length	= 0L;

		if(!ChmpxObj.Receive(&pComPkt, &pbody, &length, 1000, true)){		// timeout 1s, auto rejoin
			ERR_CHMPRN("Something error occured at waiting message.");
			break;
		}
		if(!pComPkt){
			MSG_CHMPRN("Timeouted 1s for waiting message, so continue to waiting.");
		}else if(!pbody){
			ERR_CHMPRN("NULL body data recieved.");
			CHM_Free(pComPkt);
			break;
		}else{
			MSG_CHMPRN("Succeed to receive message.");

			PBCOM	pBinCom = reinterpret_cast<PBCOM>(pbody);

			// check
			if(length != scom_total_length(pBinCom->scom)){
				ERR_CHMPRN("Length(%zu) of recieved message body is wrong(should be %zu).", length, scom_total_length(pBinCom->scom));
				CHM_Free(pComPkt);
				CHM_Free(pbody);
				break;
			}

			// output
			string	Mode	= &(pBinCom->scom.szCommand[1]);			// First charactor is "\n", so skip it
			char*	pkey	= 0UL < pBinCom->scom.key_length	? GetPrintableString(&(pBinCom->byData[pBinCom->scom.key_pos]), pBinCom->scom.key_length)		: NULL;
			char*	pval	= 0UL < pBinCom->scom.val_length	? GetPrintableString(&(pBinCom->byData[pBinCom->scom.val_pos]), pBinCom->scom.val_length)		: NULL;
			char*	psubkey	= 0UL < pBinCom->scom.skey_length	? GetPrintableString(&(pBinCom->byData[pBinCom->scom.skey_pos]), pBinCom->scom.skey_length)		: NULL;
			char*	pattr	= 0UL < pBinCom->scom.attr_length	? GetPrintableString(&(pBinCom->byData[pBinCom->scom.attrs_pos]), pBinCom->scom.attr_length)	: NULL;
			char*	pexdata	= 0UL < pBinCom->scom.exdata_length	? GetPrintableString(&(pBinCom->byData[pBinCom->scom.exdata_pos]), pBinCom->scom.exdata_length)	: NULL;
			string	key		= pkey		? pkey		: "";
			string	val		= pval		? pval		: "";
			string	subkey	= psubkey	? psubkey	: "";
			string	attr	= pattr		? pattr		: "";
			string	exdata	= pexdata	? pexdata	: "";

			cout << "length:   "	<< length;
			cout << ", type:   "	<< Mode;
			cout << ", key:    "	<< key;
			cout << ", val:    "	<< val;
			cout << ", subkey: "	<< subkey << endl;
			cout << ", attr:   "	<< attr << endl;
			cout << ", exdata: "	<< exdata << endl;

			CHM_Free(pkey);
			CHM_Free(pval);
			CHM_Free(psubkey);
			CHM_Free(pattr);
			CHM_Free(pexdata);
			CHM_Free(pComPkt);
			CHM_Free(pbody);
		}
	}

	exit(EXIT_SUCCESS);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
