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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <k2hash/k2hutil.h>
#include <k2hash/k2hdbg.h>
#include <chmpx/chmutil.h>
#include <chmpx/chmconfutil.h>

#include <fstream>

#include "k2htpdtorsvrparser.h"
#include "k2htpdtorfile.h"

using namespace std;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	INI_INCLUDE_STR						"INCLUDE"
#define	INI_KV_SEP_CHAR						'='
#define	INI_VALUE_AREA_CHAR					'\"'
#define	INI_COMMENT_CHAR					'#'
#define	INI_SEC_START_CHAR					'['
#define	INI_SEC_END_CHAR					']'

// section key words
#define	INI_K2HDTORSVR_MAIN_STR				"K2HTPDTORSVR"
#define	INI_K2HDTORSVR_MAIN_SEC_STR			"[" INI_K2HDTORSVR_MAIN_STR "]"

// key words in section
#define	INI_K2HDTORSVR_OUTPUTFILE_STR		"OUTPUTFILE"

#define	INI_K2HDTORSVR_PLUGIN_STR			"PLUGIN"

#define	INI_K2HDTORSVR_K2HTYPE_STR			"K2HTYPE"
#define	INI_K2HDTORSVR_K2HFILE_STR			"K2HFILE"
#define	INI_K2HDTORSVR_K2HFULLMAP_STR		"K2HFULLMAP"
#define	INI_K2HDTORSVR_K2HINIT_STR			"K2HINIT"
#define	INI_K2HDTORSVR_K2HMASKBIT_STR		"K2HMASKBIT"
#define	INI_K2HDTORSVR_K2HCMASKBIT_STR		"K2HCMASKBIT"
#define	INI_K2HDTORSVR_K2HMAXELE_STR		"K2HMAXELE"
#define	INI_K2HDTORSVR_K2HPAGESIZE_STR		"K2HPAGESIZE"

#define	INI_K2HDTORSVR_K2HDTORSVR_TRANS_STR	"K2HDTORSVR_TRANS"
#define	INI_K2HDTORSVR_DTORTHREADCNT_STR	"DTORTHREADCNT"

// value words
#define	INI_K2HDTORSVR_MEM1_VAL_STR			"M"
#define	INI_K2HDTORSVR_MEM2_VAL_STR			"MEM"
#define	INI_K2HDTORSVR_MEM3_VAL_STR			"MEMORY"
#define	INI_K2HDTORSVR_FILE1_VAL_STR		"F"
#define	INI_K2HDTORSVR_FILE2_VAL_STR		"FILE"
#define	INI_K2HDTORSVR_TEMP1_VAL_STR		"T"
#define	INI_K2HDTORSVR_TEMP2_VAL_STR		"TEMP"
#define	INI_K2HDTORSVR_TEMP3_VAL_STR		"TEMPORARY"

#define	INI_K2HDTORSVR_YES1_VAL_STR			"Y"
#define	INI_K2HDTORSVR_YES2_VAL_STR			"YES"
#define	INI_K2HDTORSVR_NO1_VAL_STR			"N"
#define	INI_K2HDTORSVR_NO2_VAL_STR			"NO"
#define	INI_K2HDTORSVR_ON_VAL_STR			"ON"
#define	INI_K2HDTORSVR_OFF_VAL_STR			"OFF"

// Environment
#define	DTORSVR_CONFFILE_ENV_NAME			"DTORSVRCONFFILE"
#define	DTORSVR_JSONCONF_ENV_NAME			"DTORSVRJSONCONF"

//---------------------------------------------------------
// Utilities
//---------------------------------------------------------
static bool read_ini_file_contents(const char* path, strlst_t& lines, strlst_t& files)
{
	if(ISEMPTYSTR(path)){
		ERR_K2HPRN("path is empty.");
		return false;
	}

	// check file path
	string	realpath;
	if(!cvt_path_real_path(path, realpath)){
		ERR_K2HPRN("Could not convert realpath from file(%s)", path);
		return false;
	}
	// is already listed
	for(strlst_t::const_iterator iter = files.begin(); iter != files.end(); ++iter){
		if(realpath == (*iter)){
			ERR_K2HPRN("file(%s:%s) is already loaded.", path, realpath.c_str());
			return false;
		}
	}
	files.push_back(realpath);

	// load
	ifstream	cfgstream(realpath.c_str(), ios::in);
	if(!cfgstream.good()){
		ERR_K2HPRN("Could not open(read only) file(%s:%s)", path, realpath.c_str());
		return false;
	}

	string		line;
	int			lineno;
	for(lineno = 1; cfgstream.good() && getline(cfgstream, line); lineno++){
		line = trim(line);
		if(0 == line.length()){
			continue;
		}
		if(INI_COMMENT_CHAR == line[0]){
			continue;
		}

		// check only include
		string::size_type	pos;
		if(string::npos != (pos = line.find(INI_INCLUDE_STR))){
			string	value	= trim(line.substr(pos + 1));
			string	key		= trim(line.substr(0, pos));
			if(key == INI_INCLUDE_STR){
				// found include, do reentrant
				if(!read_ini_file_contents(value.c_str(), lines, files)){
					ERR_K2HPRN("Failed to load include file(%s)", value.c_str());
					cfgstream.close();
					return false;
				}
				continue;
			}
		}
		// add
		lines.push_back(line);
	}
	cfgstream.close();

	return true;
}

static void parse_ini_line(const string& line, string& key, string& value)
{
	string::size_type	pos;
	if(string::npos != (pos = line.find(INI_KV_SEP_CHAR))){
		key		= trim(line.substr(0, pos));
		value	= trim(line.substr(pos + 1));
	}else{
		key		= trim(line);
		value	= "";
	}
	if(!extract_conf_value(key) || !extract_conf_value(value)){
		key.clear();
		value.clear();
	}else{
		key		= upper(key);								// key convert to upper
	}
}

inline void initialize_k2htpdtorsvr_info(PK2HTPDTORSVRINFO pInfo)
{
	if(!pInfo){
		return;
	}
	pInfo->conffile					= "";
	pInfo->output_file				= "";
	pInfo->plugin_param				= "";
	pInfo->is_k2hash				= false;
	pInfo->k2hash_type_mem			= false;
	pInfo->k2hash_type_tmp			= false;
	pInfo->k2hash_file				= "";
	pInfo->k2hash_fullmap			= false;
	pInfo->k2hash_init				= false;
	pInfo->k2hash_mask_bitcnt		= K2HShm::DEFAULT_MASK_BITCOUNT;
	pInfo->k2hash_cmask_bitcnt		= K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT;
	pInfo->k2hash_max_element_cnt	= K2HShm::DEFAULT_MAX_ELEMENT_CNT;
	pInfo->k2hash_pagesize			= K2HShm::MIN_PAGE_SIZE;
	pInfo->trans_ini_file			= "";
	pInfo->trans_dtor_threads		= K2HTransManager::DEFAULT_THREAD_POOL;
}

static bool ParseK2htpdtorsvrIniFile(const char* conffile, PK2HTPDTORSVRINFO pInfo)
{
	if(ISEMPTYSTR(conffile) || !pInfo){
		ERR_K2HPRN("parameters are wrong.");
		return false;
	}
	// read conf file
	strlst_t	lines;
	strlst_t	files;
	if(!read_ini_file_contents(conffile, lines, files)){
		ERR_K2HPRN("Failed to load configuration ini file(%s)", conffile);
		return false;
	}

	// initialize
	initialize_k2htpdtorsvr_info(pInfo);

	// Parse & Check
	bool	set_main_sec = false;
	for(strlst_t::const_iterator iter = lines.begin(); iter != lines.end(); ){
		if((*iter) == INI_K2HDTORSVR_MAIN_SEC_STR){
			// main section
			if(set_main_sec){
				ERR_K2HPRN("main section(%s) is already set.", INI_K2HDTORSVR_MAIN_SEC_STR);
				return false;
			}
			set_main_sec = true;

			// loop for main section
			for(++iter; iter != lines.end(); ++iter){
				if(INI_SEC_START_CHAR == (*iter)[0] && INI_SEC_END_CHAR == (*iter)[iter->length() - 1]){
					// another section start, so break this loop
					break;
				}

				// parse key(to upper) & value
				string	key;
				string	value;
				parse_ini_line((*iter), key, value);

				// compare key word
				if(INI_K2HDTORSVR_OUTPUTFILE_STR == key){
					// output file path
					pInfo->output_file = value;

				}else if(INI_K2HDTORSVR_PLUGIN_STR == key){
					// plugin path
					pInfo->plugin_param = value;

				}else if(INI_K2HDTORSVR_K2HTYPE_STR == key){
					// k2hash type
					pInfo->is_k2hash = true;
					value = upper(value);
					if(INI_K2HDTORSVR_MEM1_VAL_STR == value || INI_K2HDTORSVR_MEM2_VAL_STR == value || INI_K2HDTORSVR_MEM3_VAL_STR == value){
						pInfo->k2hash_type_mem = true;
						pInfo->k2hash_type_tmp = false;
					}else if(INI_K2HDTORSVR_TEMP1_VAL_STR == value || INI_K2HDTORSVR_TEMP2_VAL_STR == value || INI_K2HDTORSVR_TEMP3_VAL_STR == value){
						pInfo->k2hash_type_mem = false;
						pInfo->k2hash_type_tmp = true;
					}else if(INI_K2HDTORSVR_FILE1_VAL_STR == value || INI_K2HDTORSVR_FILE2_VAL_STR == value){
						pInfo->k2hash_type_mem = false;
						pInfo->k2hash_type_tmp = false;
					}else{
						WAN_K2HPRN("keyword(%s)'s value(%s) in main section(%s) is unknown value, so skip it.", key.c_str(), value.c_str(), INI_K2HDTORSVR_MAIN_SEC_STR);
					}

				}else if(INI_K2HDTORSVR_K2HFILE_STR == key){
					// k2hash file
					pInfo->is_k2hash		= true;
					pInfo->k2hash_type_mem	= false;
					pInfo->k2hash_file		= value;

				}else if(INI_K2HDTORSVR_K2HFULLMAP_STR == key){
					// k2hash fullmap
					pInfo->is_k2hash = true;
					value = upper(value);
					if(INI_K2HDTORSVR_YES1_VAL_STR == value || INI_K2HDTORSVR_YES2_VAL_STR == value || INI_K2HDTORSVR_ON_VAL_STR == value){
						pInfo->k2hash_fullmap = true;
					}else if(INI_K2HDTORSVR_NO1_VAL_STR == value || INI_K2HDTORSVR_NO2_VAL_STR == value || INI_K2HDTORSVR_OFF_VAL_STR == value){
						pInfo->k2hash_fullmap = false;
					}else{
						WAN_K2HPRN("keyword(%s)'s value(%s) in main section(%s) is unknown value, so skip it.", key.c_str(), value.c_str(), INI_K2HDTORSVR_MAIN_SEC_STR);
					}

				}else if(INI_K2HDTORSVR_K2HINIT_STR == key){
					// k2hash init
					pInfo->is_k2hash = true;
					value = upper(value);
					if(INI_K2HDTORSVR_YES1_VAL_STR == value || INI_K2HDTORSVR_YES2_VAL_STR == value || INI_K2HDTORSVR_ON_VAL_STR == value){
						pInfo->k2hash_init = true;
					}else if(INI_K2HDTORSVR_NO1_VAL_STR == value || INI_K2HDTORSVR_NO2_VAL_STR == value || INI_K2HDTORSVR_OFF_VAL_STR == value){
						pInfo->k2hash_init = false;
					}else{
						WAN_K2HPRN("keyword(%s)'s value(%s) in main section(%s) is unknown value, so skip it.", key.c_str(), value.c_str(), INI_K2HDTORSVR_MAIN_SEC_STR);
					}

				}else if(INI_K2HDTORSVR_K2HMASKBIT_STR == key){
					// k2hash MaskBitCnt
					pInfo->k2hash_mask_bitcnt = atoi(value.c_str());

				}else if(INI_K2HDTORSVR_K2HCMASKBIT_STR == key){
					// k2hash CMaskBitCnt
					pInfo->k2hash_cmask_bitcnt = atoi(value.c_str());

				}else if(INI_K2HDTORSVR_K2HMAXELE_STR == key){
					// k2hash MaxElementCnt
					pInfo->k2hash_max_element_cnt = atoi(value.c_str());

				}else if(INI_K2HDTORSVR_K2HPAGESIZE_STR == key){
					// k2hash PageSize
					pInfo->k2hash_pagesize = static_cast<size_t>(atoll(value.c_str()));

				}else if(INI_K2HDTORSVR_K2HDTORSVR_TRANS_STR == key){
					// transfer
					pInfo->trans_ini_file = "";
					if(!value.empty() && right_check_json_string(value.c_str())){
						// json string
						pInfo->trans_ini_file = value;
					}else{
						// not json string, thus it is a file
						if(!cvt_path_real_path(value.c_str(), pInfo->trans_ini_file)){
							WAN_K2HPRN("keyword(%s) has value(%s) in main section(%s), but does not file exist.", key.c_str(), value.c_str(), INI_K2HDTORSVR_MAIN_SEC_STR);
						}
					}
				}else if(INI_K2HDTORSVR_DTORTHREADCNT_STR == key){
					// dtor thread pool count
					pInfo->trans_dtor_threads = atoi(value.c_str());
				}
			}
		}else{
			++iter;
		}
	}

	// check values
	if(pInfo->output_file.empty() && pInfo->plugin_param.empty() && !pInfo->is_k2hash){
		ERR_K2HPRN("configuration file has no plugin and no k2hash output.");
		return false;
	}
	if(pInfo->is_k2hash){
		if(!(pInfo->k2hash_type_mem) && !(pInfo->k2hash_type_tmp) && pInfo->k2hash_file.empty()){
			ERR_K2HPRN("configuration file specify k2hash output, but no k2hash type.");
			return false;
		}
		if(pInfo->k2hash_type_mem){
			if(pInfo->k2hash_type_tmp || !pInfo->k2hash_file.empty()){
				ERR_K2HPRN("configuration file specify k2hash memory output, but k2hash type conflict with temporary or file.");
				return false;
			}
			if(!pInfo->k2hash_fullmap){
				ERR_K2HPRN("configuration file specify k2hash memory output, but k2hash does not full mapping.");
				return false;
			}
			if(!pInfo->k2hash_init){
				ERR_K2HPRN("configuration file specify k2hash memory output, but k2hash does not initialize.");
				return false;
			}
		}
		if(pInfo->k2hash_type_tmp){
			if(!pInfo->k2hash_fullmap){
				ERR_K2HPRN("configuration file specify k2hash temporary output, but k2hash does not full mapping.");
				return false;
			}
			if(!pInfo->k2hash_init){
				ERR_K2HPRN("configuration file specify k2hash temporary output, but k2hash does not initialize.");
				return false;
			}
		}
		if(!pInfo->trans_ini_file.empty()){
			if(pInfo->trans_dtor_threads < K2HTransManager::DEFAULT_THREAD_POOL){
				ERR_K2HPRN("configuration file specify transfer setting, but trans(dtor) thread count(%d) is wrong.", pInfo->trans_dtor_threads);
				return false;
			}
		}
	}
	return true;
}

static bool ParseK2htpdtorsvrYamlContents(yaml_parser_t& yparser, PK2HTPDTORSVRINFO pInfo)
{
	// Must start yaml mapping event.
	yaml_event_t	yevent;
	if(!yaml_parser_parse(&yparser, &yevent)){
		ERR_K2HPRN("Could not parse event. errno = %d", errno);
		return false;
	}
	if(YAML_MAPPING_START_EVENT != yevent.type){
		ERR_K2HPRN("Parsed event type is not start mapping(%d)", yevent.type);
		yaml_event_delete(&yevent);
		return false;
	}
	yaml_event_delete(&yevent);

	// Loading
	string	key("");
	bool	result = true;
	for(bool is_loop = true; is_loop && result; ){
		// get event
		if(!yaml_parser_parse(&yparser, &yevent)){
			ERR_K2HPRN("Could not parse event. errno = %d", errno);
			result = false;
			continue;
		}

		// check event
		if(YAML_MAPPING_END_EVENT == yevent.type){
			// End of mapping event
			is_loop = false;

		}else if(YAML_SCALAR_EVENT == yevent.type){
			// Load key & value
			if(key.empty()){
				key = reinterpret_cast<const char*>(yevent.data.scalar.value);

			}else if(0 != strlen(reinterpret_cast<const char*>(yevent.data.scalar.value))){
				//
				// Compare key and set value
				//
				if(0 == strcasecmp(INI_K2HDTORSVR_OUTPUTFILE_STR, key.c_str())){
					// output file path
					pInfo->output_file = reinterpret_cast<const char*>(yevent.data.scalar.value);

				}else if(0 == strcasecmp(INI_K2HDTORSVR_PLUGIN_STR, key.c_str())){
					// plugin path
					pInfo->plugin_param = reinterpret_cast<const char*>(yevent.data.scalar.value);

				}else if(0 == strcasecmp(INI_K2HDTORSVR_K2HTYPE_STR, key.c_str())){
					// k2hash type
					const char*	pvalue	= reinterpret_cast<const char*>(yevent.data.scalar.value);
					pInfo->is_k2hash	= true;
					if(0 == strcasecmp(INI_K2HDTORSVR_MEM1_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_MEM2_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_MEM3_VAL_STR, pvalue)){
						pInfo->k2hash_type_mem = true;
						pInfo->k2hash_type_tmp = false;
					}else if(0 == strcasecmp(INI_K2HDTORSVR_TEMP1_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_TEMP2_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_TEMP3_VAL_STR, pvalue)){
						pInfo->k2hash_type_mem = false;
						pInfo->k2hash_type_tmp = true;
					}else if(0 == strcasecmp(INI_K2HDTORSVR_FILE1_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_FILE2_VAL_STR, pvalue)){
						pInfo->k2hash_type_mem = false;
						pInfo->k2hash_type_tmp = false;
					}else{
						WAN_K2HPRN("keyword(%s)'s value(%s) in main section(%s) is unknown value, so skip it.", key.c_str(), pvalue, INI_K2HDTORSVR_MAIN_SEC_STR);
					}

				}else if(0 == strcasecmp(INI_K2HDTORSVR_K2HFILE_STR, key.c_str())){
					// k2hash file
					pInfo->is_k2hash		= true;
					pInfo->k2hash_type_mem	= false;
					pInfo->k2hash_file		= reinterpret_cast<const char*>(yevent.data.scalar.value);

				}else if(0 == strcasecmp(INI_K2HDTORSVR_K2HFULLMAP_STR, key.c_str())){
					// k2hash fullmap
					const char*	pvalue	= reinterpret_cast<const char*>(yevent.data.scalar.value);
					pInfo->is_k2hash	= true;

					if(0 == strcasecmp(INI_K2HDTORSVR_YES1_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_YES2_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_ON_VAL_STR, pvalue)){
						pInfo->k2hash_fullmap = true;
					}else if(0 == strcasecmp(INI_K2HDTORSVR_NO1_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_NO2_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_OFF_VAL_STR, pvalue)){
						pInfo->k2hash_fullmap = false;
					}else{
						WAN_K2HPRN("keyword(%s)'s value(%s) in main section(%s) is unknown value, so skip it.", key.c_str(), pvalue, INI_K2HDTORSVR_MAIN_SEC_STR);
					}

				}else if(0 == strcasecmp(INI_K2HDTORSVR_K2HINIT_STR, key.c_str())){
					// k2hash init
					const char*	pvalue	= reinterpret_cast<const char*>(yevent.data.scalar.value);
					pInfo->is_k2hash	= true;
					if(0 == strcasecmp(INI_K2HDTORSVR_YES1_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_YES2_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_ON_VAL_STR, pvalue)){
						pInfo->k2hash_init = true;
					}else if(0 == strcasecmp(INI_K2HDTORSVR_NO1_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_NO2_VAL_STR, pvalue) || 0 == strcasecmp(INI_K2HDTORSVR_OFF_VAL_STR, pvalue)){
						pInfo->k2hash_init = false;
					}else{
						WAN_K2HPRN("keyword(%s)'s value(%s) in main section(%s) is unknown value, so skip it.", key.c_str(), pvalue, INI_K2HDTORSVR_MAIN_SEC_STR);
					}

				}else if(0 == strcasecmp(INI_K2HDTORSVR_K2HMASKBIT_STR, key.c_str())){
					// k2hash MaskBitCnt
					pInfo->k2hash_mask_bitcnt = atoi(reinterpret_cast<const char*>(yevent.data.scalar.value));

				}else if(0 == strcasecmp(INI_K2HDTORSVR_K2HCMASKBIT_STR, key.c_str())){
					// k2hash CMaskBitCnt
					pInfo->k2hash_cmask_bitcnt = atoi(reinterpret_cast<const char*>(yevent.data.scalar.value));

				}else if(0 == strcasecmp(INI_K2HDTORSVR_K2HMAXELE_STR, key.c_str())){
					// k2hash MaxElementCnt
					pInfo->k2hash_max_element_cnt = atoi(reinterpret_cast<const char*>(yevent.data.scalar.value));

				}else if(0 == strcasecmp(INI_K2HDTORSVR_K2HPAGESIZE_STR, key.c_str())){
					// k2hash PageSize
					pInfo->k2hash_pagesize = static_cast<size_t>(atoll(reinterpret_cast<const char*>(yevent.data.scalar.value)));

				}else if(0 == strcasecmp(INI_K2HDTORSVR_K2HDTORSVR_TRANS_STR, key.c_str())){
					// transfer
					pInfo->trans_ini_file = "";
					if(yevent.data.scalar.value && right_check_json_string(reinterpret_cast<const char*>(yevent.data.scalar.value))){
						// json string
						pInfo->trans_ini_file = reinterpret_cast<const char*>(yevent.data.scalar.value);
					}else{
						// not json string, thus it is a file
						if(!cvt_path_real_path(reinterpret_cast<const char*>(yevent.data.scalar.value), pInfo->trans_ini_file)){
							WAN_K2HPRN("keyword(%s) has value(%s) in main section(%s), but does not file exist.", key.c_str(), reinterpret_cast<const char*>(yevent.data.scalar.value), INI_K2HDTORSVR_MAIN_SEC_STR);
						}
					}

				}else if(0 == strcasecmp(INI_K2HDTORSVR_DTORTHREADCNT_STR, key.c_str())){
					// dtor thread pool count
					pInfo->trans_dtor_threads = atoi(reinterpret_cast<const char*>(yevent.data.scalar.value));

				}else{
					// unknown key name.
					MSG_K2HPRN("Unknown key(%s), so skip this line.", key.c_str());
				}
				key.clear();
			}else{
				WAN_K2HPRN("The value is empty for %s key, so skip this line.", key.c_str());
			}
		}else{
			// [TODO] Now not support alias(anchor) event
			//
			ERR_K2HPRN("Found unexpected yaml event(%d) in %s section.", yevent.type, INI_K2HDTORSVR_MAIN_STR);
			result = false;
		}

		// delete event
		if(is_loop){
			is_loop = yevent.type != YAML_STREAM_END_EVENT;
		}
		yaml_event_delete(&yevent);
	}
	return result;
}

static bool ParseK2htpdtorsvrYamlTopLevel(yaml_parser_t& yparser, PK2HTPDTORSVRINFO pInfo)
{
	CHMYamlDataStack	other_stack;
	bool				is_set_main	= false;
	bool				result		= true;
	for(bool is_loop = true, in_stream = false, in_document = false, in_toplevel = false; is_loop && result; ){
		// get event
		yaml_event_t	yevent;
		if(!yaml_parser_parse(&yparser, &yevent)){
			ERR_K2HPRN("Could not parse event. errno = %d", errno);
			result = false;
			continue;
		}

		// check event
		switch(yevent.type){
			case YAML_NO_EVENT:
				MSG_K2HPRN("There is no yaml event in loop");
				break;

			case YAML_STREAM_START_EVENT:
				if(!other_stack.empty()){
					MSG_K2HPRN("Found start yaml stream event in skipping event loop");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else if(in_stream){
					MSG_K2HPRN("Already start yaml stream event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else{
					MSG_K2HPRN("Start yaml stream event in loop");
					in_stream = true;
				}
				break;

			case YAML_STREAM_END_EVENT:
				if(!other_stack.empty()){
					MSG_K2HPRN("Found stop yaml stream event in skipping event loop");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else if(!in_stream){
					MSG_K2HPRN("Already stop yaml stream event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else{
					MSG_K2HPRN("Stop yaml stream event in loop");
					in_stream = false;
				}
				break;

			case YAML_DOCUMENT_START_EVENT:
				if(!other_stack.empty()){
					MSG_K2HPRN("Found start yaml document event in skipping event loop");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else if(!in_stream){
					MSG_K2HPRN("Found start yaml document event before yaml stream event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else if(in_document){
					MSG_K2HPRN("Already start yaml document event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else{
					MSG_K2HPRN("Start yaml document event in loop");
					in_document = true;
				}
				break;

			case YAML_DOCUMENT_END_EVENT:
				if(!other_stack.empty()){
					MSG_K2HPRN("Found stop yaml document event in skipping event loop");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else if(!in_document){
					MSG_K2HPRN("Already stop yaml document event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else{
					MSG_K2HPRN("Stop yaml document event in loop");
					in_document = false;
				}
				break;

			case YAML_MAPPING_START_EVENT:
				if(!other_stack.empty()){
					MSG_K2HPRN("Found start yaml mapping event in skipping event loop");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else if(!in_stream){
					MSG_K2HPRN("Found start yaml mapping event before yaml stream event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else if(!in_document){
					MSG_K2HPRN("Found start yaml mapping event before yaml document event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else if(in_toplevel){
					MSG_K2HPRN("Already start yaml mapping event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else{
					MSG_K2HPRN("Start yaml mapping event in loop");
					in_toplevel = true;
				}
				break;

			case YAML_MAPPING_END_EVENT:
				if(!other_stack.empty()){
					MSG_K2HPRN("Found stop yaml mapping event in skipping event loop");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else if(!in_toplevel){
					MSG_K2HPRN("Already stop yaml mapping event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else{
					MSG_K2HPRN("Stop yaml mapping event in loop");
					in_toplevel = false;
				}
				break;

			case YAML_SEQUENCE_START_EVENT:
				// always stacking
				//
				if(!other_stack.empty()){
					MSG_K2HPRN("Found start yaml sequence event in skipping event loop");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else{
					MSG_K2HPRN("Found start yaml sequence event before top level event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}
				break;

			case YAML_SEQUENCE_END_EVENT:
				// always stacking
				//
				if(!other_stack.empty()){
					MSG_K2HPRN("Found stop yaml sequence event in skipping event loop");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else{
					MSG_K2HPRN("Found stop yaml sequence event before top level event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}
				break;

			case YAML_SCALAR_EVENT:
				if(!other_stack.empty()){
					MSG_K2HPRN("Got yaml scalar event in skipping event loop");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else if(!in_stream){
					MSG_K2HPRN("Got yaml scalar event before yaml stream event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else if(!in_document){
					MSG_K2HPRN("Got yaml scalar event before yaml document event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else if(!in_toplevel){
					MSG_K2HPRN("Got yaml scalar event before yaml mapping event in loop, Thus stacks this event.");
					if(!other_stack.add(yevent.type)){
						result = false;
					}
				}else{
					// Found Top Level Keywords, start to loading
					if(0 == strcasecmp(INI_K2HDTORSVR_MAIN_STR, reinterpret_cast<const char*>(yevent.data.scalar.value))){
						if(is_set_main){
							MSG_K2HPRN("Got yaml scalar event in loop, but already loading %s. Thus stacks this event.", INI_K2HDTORSVR_MAIN_STR);
							if(!other_stack.add(yevent.type)){
								result = false;
							}
						}else{
							// Load K2HTPDTORSVR section
							if(!ParseK2htpdtorsvrYamlContents(yparser, pInfo)){
								ERR_K2HPRN("Something error occured in loading %s section.", INI_K2HDTORSVR_MAIN_STR);
								result = false;
							}
						}

					}else{
						MSG_K2HPRN("Got yaml scalar event in loop, but unknown keyword(%s) for me. Thus stacks this event.", reinterpret_cast<const char*>(yevent.data.scalar.value));
						if(!other_stack.add(yevent.type)){
							result = false;
						}
					}
				}
				break;

			case YAML_ALIAS_EVENT:
				// [TODO]
				// Now we do not supports alias(anchor) event.
				//
				MSG_K2HPRN("Got yaml alias(anchor) event in loop, but we does not support this event. Thus skip this event.");
				break;
		}
		// delete event
		is_loop = yevent.type != YAML_STREAM_END_EVENT;
		yaml_event_delete(&yevent);
	}
	return result;
}

static bool ParseK2htpdtorsvrYaml(const char* config, PK2HTPDTORSVRINFO pInfo, bool is_json_string)
{
	// initialize yaml parser
	yaml_parser_t	yparser;
	if(!yaml_parser_initialize(&yparser)){
		ERR_K2HPRN("Failed to initialize yaml parser");
		return false;
	}

	FILE*	fp = NULL;
	if(!is_json_string){
		// open configuration file
		if(NULL == (fp = fopen(config, "r"))){
			ERR_K2HPRN("Could not open configuration file(%s). errno = %d", config, errno);
			// cppcheck-suppress resourceLeak
			return false;
		}
		// set file to parser
		yaml_parser_set_input_file(&yparser, fp);

	}else{	// JSON_STR
		// set string to parser
		yaml_parser_set_input_string(&yparser, reinterpret_cast<const unsigned char*>(config), strlen(config));
	}

	// Do parsing
	bool	result = ParseK2htpdtorsvrYamlTopLevel(yparser, pInfo);

	yaml_parser_delete(&yparser);
	if(fp){
		fclose(fp);
	}
	return result;
}

bool ParseK2htpdtorsvrConfiguration(const char* config, PK2HTPDTORSVRINFO pInfo)
{
	// get configuration type without environment
	string		normalize_config("");
	CHMCONFTYPE	conftype = check_chmconf_type_ex(config, DTORSVR_CONFFILE_ENV_NAME, DTORSVR_JSONCONF_ENV_NAME, &normalize_config);
	if(CHMCONF_TYPE_UNKNOWN == conftype || CHMCONF_TYPE_NULL == conftype){
		ERR_K2HPRN("configuration file or json string is wrong.");
		return false;
	}

	bool	result;
	if(CHMCONF_TYPE_INI_FILE == conftype){
		result = ParseK2htpdtorsvrIniFile(normalize_config.c_str(), pInfo);
	}else{
		result = ParseK2htpdtorsvrYaml(normalize_config.c_str(), pInfo, (CHMCONF_TYPE_JSON_STRING == conftype));
	}

	// configuration file
	pInfo->conffile = normalize_config;

	return result;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
