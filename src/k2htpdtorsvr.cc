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
#include <errno.h>

#include <k2hash/k2hash.h>
#include <k2hash/k2hutil.h>
#include <k2hash/k2hdbg.h>
#include <chmpx/chmpx.h>

#include <string>
#include <map>

#include "k2htpdtorsvrman.h"
#include "k2htpdtorsvrparser.h"

using namespace std;

//---------------------------------------------------------
// Types
//---------------------------------------------------------
typedef std::map<std::string, std::string>	dtorparam_t;

//---------------------------------------------------------
// Variable
//---------------------------------------------------------
static volatile bool	is_doing_loop = true;

//---------------------------------------------------------
// Version
//---------------------------------------------------------
extern char k2htpdtorsvr_commit_hash[];

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
// [Usage]
// -h                help display
// -ver              version display
// -conf <ini file>  configuration file for k2htpdtorsvr processing
// -d <debug level>  debugging level: ERR(default) / WAN / INFO / SILENT
// -dlog <file path> output file for debugging message(default stderr)
// 
static void PrintUsage(char* argv)
{
	string	prgname = argv ? basename(argv) : "k2htpdtorserver";

	cout << "[Usage]" << endl;
	cout << prgname << " -h" << endl;
	cout << prgname << " [-conf <path> | -json <string>] [-d <debug level>] [-dlog <file path>]" << endl;
	cout << endl;
	cout << "-h                     help display" << endl;
	cout << "-ver                   version display" << endl;
	cout << "-conf <path>           configuration file(.ini .yaml .json) path" << endl;
	cout << "-json <json string>    configuration json string" << endl;
	cout << "-d <debug level>       debugging level: ERR(default) / WAN / INFO(MSG) / SILENT" << endl;
	cout << "-dlog <file path>      output file for debugging message(default stderr)" << endl;
	cout << endl;
}

static void PrintVersion(FILE* stream)
{
	static const char format[] =
		"\n"
		"k2htpdtorsvr Version %s (commit: %s)\n"
		"\n"
		"Copyright(C) 2015 Yahoo Japan Corporation.\n"
		"\n"
		"K2HASH TRANSACTION PLUGIN is programmable I/F for processing\n"
		"transaction data from modifying K2HASH data.\n"
		"\n";

	if(!stream){
		stream = stdout;
	}
	fprintf(stream, format, VERSION, k2htpdtorsvr_commit_hash);

	chmpx_print_version(stream);
	k2h_print_version(stream);
}

static bool ParamParser(int argc, char** argv, dtorparam_t& params)
{
	params.clear();

	bool	is_conf_opt	= false;
	bool	is_json_opt	= false;
	for(int cnt = 1; cnt < argc; cnt++){
		if(0 == strcasecmp(argv[cnt], "-h") || 0 == strcasecmp(argv[cnt], "-help")){
			params["h"] = "";

		}else if(0 == strcasecmp(argv[cnt], "-ver") || 0 == strcasecmp(argv[cnt], "-version")){
			params["v"] = "";

		}else if(0 == strcasecmp(argv[cnt], "-conf")){
			if(argc <= cnt + 1){
				fprintf(stderr, "[ERROR] Option %s needs parameter.\n", argv[cnt]);
				return false;
			}
			if(!is_file_safe_exist(argv[cnt + 1])){
				fprintf(stderr, "[ERROR] %s file does not regular file.\n", argv[cnt]);
				return false;
			}
			if(is_json_opt){
				fprintf(stderr, "[ERROR] Already set -json option, option %s could not be specified with -json.\n", argv[cnt]);
				return false;
			}
			is_conf_opt		= false;
			params["conf"]	= argv[++cnt];

		}else if(0 == strcasecmp(argv[cnt], "-json")){
			if(argc <= cnt + 1){
				fprintf(stderr, "[ERROR] Option %s needs parameter.\n", argv[cnt]);
				return false;
			}
			// cppcheck-suppress unmatchedSuppression
			// cppcheck-suppress knownConditionTrueFalse
			if(is_conf_opt){
				fprintf(stderr, "[ERROR] Already set -conf option, option %s could not be specified with -conf.\n", argv[cnt]);
				return false;
			}
			is_json_opt		= false;
			params["json"]	= argv[++cnt];

		}else if(0 == strcasecmp(argv[cnt], "-d")){
			if(argc <= cnt + 1){
				fprintf(stderr, "[ERROR] Option %s needs parameter.\n", argv[cnt]);
				return false;
			}
			if(0 != strcasecmp(argv[cnt + 1], "silent") && 0 != strcasecmp(argv[cnt + 1], "err") && 0 != strcasecmp(argv[cnt + 1], "wan") && 0 != strcasecmp(argv[cnt + 1], "info") && 0 != strcasecmp(argv[cnt + 1], "msg")){
				fprintf(stderr, "[ERROR] Option %s value %s is not defined.\n", argv[cnt], argv[cnt + 1]);
				return false;
			}
			params["d"] = lower(argv[++cnt]);

		}else if(0 == strcasecmp(argv[cnt], "-dlog")){
			if(argc <= cnt + 1){
				fprintf(stderr, "[ERROR] Option %s needs parameter.\n", argv[cnt]);
				return false;
			}
			params["dlog"] = argv[++cnt];

		}else{
			fprintf(stderr, "[ERROR] Unknown option %s.\n", argv[cnt]);
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
	dtorparam_t	params;

	// Parse parameter
	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress stlSize
	if(!ParamParser(argc, argv, params) || 0 == params.size()){
		PrintUsage(argv[0]);
		exit(EXIT_FAILURE);
	}

	// help
	if(params.end() != params.find("h")){
		PrintUsage(argv[0]);
		exit(EXIT_SUCCESS);
	}
	// version
	if(params.end() != params.find("v")){
		PrintVersion(NULL);
		exit(EXIT_SUCCESS);
	}

	// debug mode / log
	if(params.end() == params.find("d")){
		k2h_set_debug_level_silent();
		chmpx_set_debug_level_silent();
	}else if(params["d"] == "err"){
		k2h_set_debug_level_error();
		chmpx_set_debug_level_error();
	}else if(params["d"] == "wan"){
		k2h_set_debug_level_warning();
		chmpx_set_debug_level_warning();
	}else if(params["d"] == "info" || params["d"] == "msg"){
		k2h_set_debug_level_message();
		chmpx_set_debug_level_message();
	}else{	// params["d"] == "silent"
		k2h_set_debug_level_silent();
		chmpx_set_debug_level_silent();
	}
	if(params.end() == params.find("dlog")){
		if(!k2h_set_debug_file(params["dlog"].c_str()) || !chmpx_set_debug_file(params["dlog"].c_str())){
			ERR_K2HPRN("Could not set debug log file(%s).", params["dlog"].c_str());
			exit(EXIT_FAILURE);
		}
	}

	// get configuration option
	string	config;
	if(params.end() != params.find("conf")){
		config = params["conf"].c_str();
	}else if(params.end() != params.find("json")){
		config = params["json"].c_str();
	}

	// make and check info from conf file
	K2HTPDTORSVRINFO	info;
	if(!ParseK2htpdtorsvrConfiguration(config.c_str(), &info)){
		ERR_K2HPRN("Something wrong in conf file(%s).", config.c_str());
		exit(EXIT_FAILURE);
	}

	// manager
	K2HtpSvrManager	SvrMan;
	if(!SvrMan.Initialize(&info)){
		ERR_K2HPRN("Failed to initialize manager object.");
		exit(EXIT_FAILURE);
	}

	// do processing
	if(!SvrMan.Processing()){
		ERR_K2HPRN("Something wrong occurred for loop.");
		SvrMan.Clean();
		exit(EXIT_FAILURE);
	}
	SvrMan.Clean();

	exit(EXIT_SUCCESS);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
