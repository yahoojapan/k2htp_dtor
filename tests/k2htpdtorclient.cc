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
 * CREATE:   Tue Mar 3 2015
 * REVISION:
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

#include <k2hash/k2hash.h>
#include <k2hash/k2hcommon.h>
#include <k2hash/k2hshm.h>
#include <k2hash/k2hdbg.h>
#include <k2hash/k2htrans.h>
#include <k2hash/k2harchive.h>

#include <string>
#include <iostream>

using namespace std;

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
//	[Usage]
//	prgname -f <archive file> [-l <transaction library>] [-p <transaction parameter>] [-c <thread pool count>]
//
static void PrintUsage(char* argv)
{
	string	prgname = argv ? basename(argv) : "k2htpdtorclient";

	cout << "[Usage]" << endl;
	cout << prgname << " -f <archive file> [-l <transaction library>] [-p <transaction parameter>] [-c <thread pool count>] [-e <expire seconds>]" << endl;
	cout << endl;
	cout << "-f <archive file>          specified archive file which is made by k2hlinetool trans command"	<< endl;
	cout << "-l <transaction library>   transaction shared library"											<< endl;
	cout << "-p <transaction parameter> parameter for transaction shared library"							<< endl;
	cout << "-c <thread pool count>     specify count of transaction thread pool"							<< endl;
	cout << "-e <expire seconds>        transaction expire time(seconds), default is not set expire time"	<< endl;
	cout << endl;
}

static bool CheckRegularFile(const char* pfile)
{
	if(!pfile){
		return false;
	}
	struct stat	st;
	if(0 != stat(pfile, &st)){
		ERR_K2HPRN("File %s does not exist.", pfile);
		return false;
	}
	if(!S_ISREG(st.st_mode)){
		ERR_K2HPRN("File %s is not regular file.", pfile);
		return false;
	}
	return true;
}

static bool ParamParser(int argc, char** argv, string& arfile, string& libfile, string& trparam, int& pcount, time_t& expire)
{
	arfile.erase();
	libfile.erase();
	trparam.erase();
	pcount = -1;
	expire = 0;

	for(int cnt = 1; cnt < argc; cnt++){
		if(0 == strcmp(argv[cnt], "-f")){
			if(argc <= cnt + 1){
				ERR_K2HPRN("Option %s needs parameter.", argv[cnt]);
				return false;
			}
			if(!arfile.empty()){
				ERR_K2HPRN("Option %s already set.", argv[cnt]);
				return false;
			}
			if(!CheckRegularFile(argv[++cnt])){
				ERR_K2HPRN("%s file does not regular file.", argv[cnt]);
				return false;
			}
			arfile = argv[cnt];

		}else if(0 == strcmp(argv[cnt], "-l")){
			if(argc <= cnt + 1){
				ERR_K2HPRN("Option %s needs parameter.", argv[cnt]);
				return false;
			}
			if(!libfile.empty()){
				ERR_K2HPRN("Option %s already set.", argv[cnt]);
				return false;
			}
			if(!CheckRegularFile(argv[++cnt])){
				ERR_K2HPRN("%s file does not regular file.", argv[cnt]);
				return false;
			}
			libfile = argv[cnt];

		}else if(0 == strcmp(argv[cnt], "-p")){
			if(argc <= cnt + 1){
				ERR_K2HPRN("Option %s needs parameter.", argv[cnt]);
				return false;
			}
			if(!trparam.empty()){
				ERR_K2HPRN("Option %s already set.", argv[cnt]);
				return false;
			}
			trparam = argv[++cnt];

		}else if(0 == strcmp(argv[cnt], "-c")){
			if(argc <= cnt + 1){
				ERR_K2HPRN("Option %s needs parameter.", argv[cnt]);
				return false;
			}
			if(-1 != pcount){
				ERR_K2HPRN("Option %s already set.", argv[cnt]);
				return false;
			}
			pcount = atoi(argv[++cnt]);

		}else if(0 == strcmp(argv[cnt], "-e")){
			if(argc <= cnt + 1){
				ERR_K2HPRN("Option %s needs parameter.", argv[cnt]);
				return false;
			}
			if(0 != expire){
				ERR_K2HPRN("Option %s already set.", argv[cnt]);
				return false;
			}
			time_t	tmptime = static_cast<time_t>(atoi(argv[++cnt]));
			if(tmptime <= 0){
				ERR_K2HPRN("Option -e must be over 0 value.");
				return false;
			}
			expire = tmptime;

		}else{
			ERR_K2HPRN("Unknown option %s.", argv[cnt]);
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------
// Main
//---------------------------------------------------------
int main(int argc, char** argv)
{
	K2HShm	k2hash;
	string	arfile;
	string	libfile;
	string	trparam;
	int		pcount = -1;
	time_t	expire = 0;

	// Parse parameter
	if(!ParamParser(argc, argv, arfile, libfile, trparam, pcount, expire) || arfile.empty()){
		PrintUsage(argv[0]);
		exit(EXIT_FAILURE);
	}

	// make archive object
	K2HArchive	archiveobj;
	if(!archiveobj.Initialize(arfile.c_str(), false)){
		ERR_K2HPRN("Something error occurred in loading %s archive file.", arfile.c_str());
		exit(EXIT_FAILURE);
	}

	// attach k2hash on memory with default parameter
	if(!k2hash.AttachMem()){
		ERR_K2HPRN("Could not initialize k2hash on memory.");
		exit(EXIT_FAILURE);
	}

	// load transaction shared library
	if(!libfile.empty()){
		if(!k2h_load_transaction_library(libfile.c_str())){
			ERR_K2HPRN("Failed to load %s transaction shared library.", libfile.c_str());
			k2hash.Detach();
			exit(EXIT_FAILURE);
		}
	}

	// set transaction pool
	if(-1 != pcount){
		if(pcount < K2HTransManager::NO_THREAD_POOL || K2HTransManager::MAX_THREAD_POOL < pcount){
			ERR_K2HPRN("Transaction thread pool count(%d) must be range %d to %d.", pcount, K2HTransManager::NO_THREAD_POOL, K2HTransManager::MAX_THREAD_POOL);
			k2hash.Detach();
			exit(EXIT_FAILURE);
		}
		if(!k2hash.SetTransThreadPool(pcount)){
			ERR_K2HPRN("Failed to set transaction thread pool %d.", pcount);
			k2hash.Detach();
			exit(EXIT_FAILURE);
		}

	}else{
		// default 1
		if(!k2hash.SetTransThreadPool(K2HTransManager::DEFAULT_THREAD_POOL)){
			ERR_K2HPRN("Failed to set transaction thread pool %d.", K2HTransManager::DEFAULT_THREAD_POOL);
			k2hash.Detach();
			exit(EXIT_FAILURE);
		}
	}

	// enable transaction
	if(!k2hash.EnableTransaction(NULL, NULL, 0, (0 == trparam.size() ? NULL : reinterpret_cast<const unsigned char*>(trparam.c_str())), (0 == trparam.size() ? 0 : trparam.length() + 1), (0 == expire ? NULL : &expire))){
		ERR_K2HPRN("Failed to enable transaction with param(%s).", trparam.c_str());
		k2hash.Detach();
		exit(EXIT_FAILURE);
	}

	// start to load archive commands
	cout << "Start to load archive file : " << arfile << endl;

	if(!archiveobj.Serialize(&k2hash, true)){
		ERR_K2HPRN("Something error occurred in loading %s archive file.", arfile.c_str());
		k2hash.Detach();
		exit(EXIT_FAILURE);
	}
	cout << "Finished to load archive file" << endl;
	cout << endl;

	// detach
	k2hash.Detach(60 * 1000);		// wait max 60s
	cout << "Finished." << endl;

	exit(EXIT_SUCCESS);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
