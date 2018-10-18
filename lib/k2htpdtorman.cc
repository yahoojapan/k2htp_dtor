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
#include <chmpx/chmpx.h>
#include <chmpx/chmutil.h>
#include <chmpx/chmconfutil.h>
#include <fullock/fullock.h>
#include <fullock/flckstructure.h>
#include <fullock/flckbaselist.tcc>

#include <string>
#include <map>
#include <list>
#include <fstream>
#include <vector>

#include "k2htpdtorman.h"
#include "k2htpdtorfile.h"

using namespace std;

//---------------------------------------------------------
// Symbols( for ini file )
//---------------------------------------------------------
#define	INICFG_INCLUDE_STR				"INCLUDE"
#define	INICFG_KV_SEP					"="
#define	INICFG_COMMENT_CHAR				'#'
#define	INICFG_SEC_START_CHAR			'['
#define	INICFG_SEC_END_CHAR				']'
#define	INICFG_VAL_SEPARATOR_CHAR		','

#define	CFG_K2HTPDTOR_SEC_STR			"K2HTPDTOR"

#define	INICFG_K2HTPDTOR_SEC_STR		"[" CFG_K2HTPDTOR_SEC_STR "]"
#define	INICFG_K2HTPDTOR_BC_STR			"K2HTPDTOR_BROADCAST"
#define	INICFG_K2HTPDTOR_CHMPXCONF_STR	"K2HTPDTOR_CHMPXCONF"
#define	INICFG_K2HTPDTOR_EXCEPTKEY_STR	"K2HTPDTOR_EXCEPT_KEY"
#define	INICFG_K2HTPDTOR_FILE_STR		"K2HTPDTOR_FILE"
#define	INICFG_K2HTPDTOR_FILTER_STR		"K2HTPDTOR_FILTER_TYPE"

#define	INICFG_K2HTPDTOR_VAL_ON			"ON"
#define	INICFG_K2HTPDTOR_VAL_OFF		"OFF"
#define	INICFG_K2HTPDTOR_VAL_YES		"YES"
#define	INICFG_K2HTPDTOR_VAL_NO			"NO"
#define	INICFG_K2HTPDTOR_FILTER_SETALL	"SETALL"
#define	INICFG_K2HTPDTOR_FILTER_DELKEY	"DELKEY"
#define	INICFG_K2HTPDTOR_FILTER_OWVAL	"OWVAL"
#define	INICFG_K2HTPDTOR_FILTER_REPVAL	"REPVAL"
#define	INICFG_K2HTPDTOR_FILTER_REPSKEY	"REPSKEY"
#define	INICFG_K2HTPDTOR_FILTER_REPATTR	"REPATTR"
#define	INICFG_K2HTPDTOR_FILTER_RENAME	"RENAME"

//---------------------------------------------------------
// Utility
//---------------------------------------------------------
// binary array
//
typedef std::vector<unsigned char>		byvec_t;

//
// Convert string with escape sequence string to binary array.
// Following string to convert:
//	"\a"	-> '\a'
//	"\b"	-> '\b'
//	"\f"	-> '\f'
//	"\n"	-> '\n'
//	"\r"	-> '\r'
//	"\t"	-> '\t'
//	"\v"	-> '\v'
//	"\'"	-> '\''
//	"\""	-> '\"'
//	"\?"	-> '\?'
//	"\\"	-> '\\'
//	"\0123"	-> "123" means octal number string
//	"\0xff"	-> "ff" means hexadecimal number string
//
// [NOTE]
// If you need to set 0x00('\0'), you can specify the string as
// "\0000" or "\0x00".
// 
static unsigned char* cvt_escape_sequence(const char* pstr, size_t& length, bool is_lastnil)
{
	length = 0;
	byvec_t	byarray;

	for(; pstr && '\0' != *pstr; pstr++){
		if('\\' == *pstr){
			if('a' == pstr[1]){
				byarray.push_back(static_cast<unsigned char>('\a'));
				pstr++;
			}else if('b' == pstr[1]){
				byarray.push_back(static_cast<unsigned char>('\b'));
				pstr++;
			}else if('f' == pstr[1]){
				byarray.push_back(static_cast<unsigned char>('\f'));
				pstr++;
			}else if('n' == pstr[1]){
				byarray.push_back(static_cast<unsigned char>('\n'));
				pstr++;
			}else if('r' == pstr[1]){
				byarray.push_back(static_cast<unsigned char>('\r'));
				pstr++;
			}else if('t' == pstr[1]){
				byarray.push_back(static_cast<unsigned char>('\t'));
				pstr++;
			}else if('v' == pstr[1]){
				byarray.push_back(static_cast<unsigned char>('\v'));
				pstr++;
			}else if('\'' == pstr[1]){
				byarray.push_back(static_cast<unsigned char>('\''));
				pstr++;
			}else if('\"' == pstr[1]){
				byarray.push_back(static_cast<unsigned char>('\"'));
				pstr++;
			}else if('\?' == pstr[1]){
				byarray.push_back(static_cast<unsigned char>('\?'));
				pstr++;
			}else if('\\' == pstr[1]){
				byarray.push_back(static_cast<unsigned char>('\\'));
				pstr++;
			}else if('0' == pstr[1]){
				if('x' == pstr[2]){
					unsigned char	byBuff	= 0x00;
					int				pos;
					for(pos = 3; '\0' != pstr[pos] && pos < 5; pos++){
						if('0' <= pstr[pos] && pstr[pos] <= '9'){
							byBuff  = byBuff * 16;
							byBuff += static_cast<unsigned char>(pstr[pos] - '0');
						}else if('A' <= pstr[pos] && pstr[pos] <= 'F'){
							byBuff  = byBuff * 16;
							byBuff += static_cast<unsigned char>(pstr[pos] - 'A') + 0x0A;
						}else if('a' <= pstr[pos] && pstr[pos] <= 'f'){
							byBuff  = byBuff * 16;
							byBuff += static_cast<unsigned char>(pstr[pos] - 'a') + 0x0A;
						}else{
							break;
						}
					}
					byarray.push_back(byBuff);
					pstr = &pstr[pos - 1];

				}else if(0 != isdigit(pstr[2])){
					unsigned char	byBuff	= 0x00;
					int				pos;
					for(pos = 2; '\0' != pstr[pos] && pos < 5; pos++){
						if('0' <= pstr[pos] && pstr[pos] <= '9'){
							byBuff  = byBuff * 8;
							byBuff += static_cast<unsigned char>(pstr[pos] - '0');
						}else{
							break;
						}
					}
					byarray.push_back(byBuff);
					pstr = &pstr[pos - 1];
				}else{
					// "\0" + "not x nor 0-9" means "\0"
					byarray.push_back(0x00);
					pstr++;
				}
			}else{
				// not skip single '\'
				byarray.push_back(static_cast<unsigned char>(*pstr));
			}
		}else{
			byarray.push_back(static_cast<unsigned char>(*pstr));
		}
	}
	if(is_lastnil && pstr && '\0' == *pstr){
		byarray.push_back(0x00);
		pstr++;
	}
	if(0 == byarray.size()){
		return NULL;
	}

	unsigned char*	pResult;
	if(NULL == (pResult = reinterpret_cast<unsigned char*>(malloc(byarray.size())))){
		ERR_K2HPRN("Could not allocate memory.");
		return NULL;
	}
	int	setpos = 0;
	for(byvec_t::const_iterator iter = byarray.begin(); iter != byarray.end(); ++iter){
		pResult[setpos++] = *iter;
	}
	length = byarray.size();

	return pResult;
}

static bool parse_except_types(const char* pexcepts, excepttypemap_t& excmap)
{
	if(ISEMPTYSTR(pexcepts)){
		WAN_K2HPRN("except type string is empty, but continue...");
		return true;
	}

	excmap.clear();
	for(string strexcept = pexcepts; !strexcept.empty(); ){
		// check separator
		string				strone;
		string::size_type	pos;
		if(string::npos != (pos = strexcept.find(INICFG_VAL_SEPARATOR_CHAR))){
			strone		= trim(strexcept.substr(0, pos));
			strexcept	= trim(strexcept.substr(pos + 1));
		}else{
			strone		= strexcept;
			strexcept.erase();
		}
		strone = upper(strone);

		// set
		if(strone == INICFG_K2HTPDTOR_FILTER_SETALL){
			excmap[SCOM_SET_ALL] = true;
		}else if(strone == INICFG_K2HTPDTOR_FILTER_DELKEY){
			excmap[SCOM_DEL_KEY] = true;
		}else if(strone == INICFG_K2HTPDTOR_FILTER_OWVAL){
			excmap[SCOM_OW_VAL] = true;
		}else if(strone == INICFG_K2HTPDTOR_FILTER_REPVAL){
			excmap[SCOM_REPLACE_VAL] = true;
		}else if(strone == INICFG_K2HTPDTOR_FILTER_REPSKEY){
			excmap[SCOM_REPLACE_SKEY] = true;
		}else if(strone == INICFG_K2HTPDTOR_FILTER_REPATTR){
			excmap[SCOM_REPLACE_ATTRS] = true;
		}else if(strone == INICFG_K2HTPDTOR_FILTER_RENAME){
			excmap[SCOM_RENAME] = true;
		}else{
			WAN_K2HPRN("unknown except type(%s), so skip it.", strone.c_str());
		}
	}
	return true;
}

//---------------------------------------------------------
// K2HtpDtorManager : Class valiables
//---------------------------------------------------------
const int	K2HtpDtorManager::SLEEP_MS;
const int	K2HtpDtorManager::WAIT_EVENT_MAX;

//---------------------------------------------------------
// K2HtpDtorManager : Class Methods
//---------------------------------------------------------
// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
K2HtpDtorManager* K2HtpDtorManager::Get(void)
{
	static K2HtpDtorManager	dtorman;				// singleton
	return &dtorman;
}

void* K2HtpDtorManager::WorkerThread(void* param)
{
	K2HtpDtorManager*	pDtorMan = reinterpret_cast<K2HtpDtorManager*>(param);
	if(!pDtorMan){
		ERR_K2HPRN("paraemter is wrong.");
		pthread_exit(NULL);
	}

	while(pDtorMan->run_thread){
		// check & add events
		if(!pDtorMan->UpdateWatch()){
			WAN_K2HPRN("Failed to check and add epoll events, but continue...");
		}
		if(!pDtorMan->run_thread){
			break;
		}

		// wait events
		struct epoll_event  events[WAIT_EVENT_MAX];
		int					eventcnt;
		if(0 < (eventcnt = epoll_pwait(pDtorMan->epollfd, events, K2HtpDtorManager::WAIT_EVENT_MAX, K2HtpDtorManager::SLEEP_MS, NULL))){		// wait 10ms as default
			if(!pDtorMan->run_thread){
				// exit ASSAP
				break;
			}
			// catch event
			for(int cnt = 0; cnt < eventcnt; cnt++){
				if(!pDtorMan->CheckWatch(events[cnt].data.fd)){
					ERR_K2HPRN("Something error occurred in checking epoll event, but continue...");
				}
			}
		}else if(-1 >= eventcnt){
			// signal occurred.
			if(EINTR != errno){
				ERR_K2HPRN("Something error occured in waiting event fd(%d) by errno(%d)", pDtorMan->epollfd, errno);
				break;
			}
		}else{	// 0 == eventcnt
			// timeouted, nothing to do
		}
	}
	pthread_exit(NULL);
	return NULL;
}

bool K2HtpDtorManager::ReadIniFileContents(const char* filepath, dtorstrlst_t& linelst, dtorstrlst_t& allfiles)
{
	if(!filepath){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}

	ifstream	cfgstream(filepath, ios::in);
	if(!cfgstream.good()){
		ERR_K2HPRN("Could not open(read only) file(%s)", filepath);
		return false;
	}

	string		line;
	int			lineno;
	for(lineno = 1; cfgstream.good() && getline(cfgstream, line); lineno++){
		line = trim(line);
		if(0 == line.length()){
			continue;
		}

		// check only include
		string::size_type	pos;
		if(string::npos != (pos = line.find(INICFG_INCLUDE_STR))){
			string	value	= trim(line.substr(pos + 1));
			string	key		= trim(line.substr(0, pos));
			if(key == INICFG_INCLUDE_STR){
				// found include.
				bool	found_same_file = false;
				for(dtorstrlst_t::const_iterator iter = allfiles.begin(); iter != allfiles.end(); ++iter){
					if(value == (*iter)){
						found_same_file = true;
						break;
					}
				}
				if(found_same_file){
					WAN_K2HPRN("%s keyword in %s(%d) is filepath(%s) which already read!", INICFG_INCLUDE_STR, filepath, lineno, value.c_str());
				}else{
					// reentrant
					allfiles.push_back(value);
					if(!K2HtpDtorManager::ReadIniFileContents(value.c_str(), linelst, allfiles)){
						ERR_K2HPRN("Failed to load include file(%s)", value.c_str());
						cfgstream.close();
						return false;
					}
				}
				continue;
			}
		}
		// add
		linelst.push_back(line);
	}
	cfgstream.close();

	return true;
}

bool K2HtpDtorManager::LoadConfigration(const char* config, string& chmpxconf, bool& isbroadcast, dtorpbylst_t& excepts, string& outputfile, excepttypemap_t& excepttypes)
{
	// get configuration type without environment
	CHMCONFTYPE	conftype = check_chmconf_type(config);
	if(CHMCONF_TYPE_UNKNOWN == conftype || CHMCONF_TYPE_NULL == conftype){
		ERR_K2HPRN("Parameter configuration file or json string is wrong.");
		return false;
	}

	bool	result;
	if(CHMCONF_TYPE_INI_FILE == conftype){
		result = K2HtpDtorManager::LoadConfigrationIni(config, chmpxconf, isbroadcast, excepts, outputfile, excepttypes);
	}else{
		result = K2HtpDtorManager::LoadConfigrationYaml(config, chmpxconf, isbroadcast, excepts, outputfile, excepttypes, (CHMCONF_TYPE_JSON_STRING == conftype));
	}
	return result;
}

bool K2HtpDtorManager::LoadConfigrationIni(const char* filepath, string& chmpxconf, bool& isbroadcast, dtorpbylst_t& excepts, string& outputfile, excepttypemap_t& excepttypes)
{
	// Load all file contents(with include file)
	dtorstrlst_t	linelst;
	dtorstrlst_t	allfiles;
	allfiles.push_back(filepath);
	if(!K2HtpDtorManager::ReadIniFileContents(filepath, linelst, allfiles)){
		ERR_K2HPRN("Could not load configration file(%s) contents.", filepath);
		return false;
	}

	chmpxconf	= filepath;			// default
	isbroadcast	= false;			// default
	outputfile	= "";				// default

	// parsing
	string		line;
	bool		is_in_section = false;
	for(dtorstrlst_t::const_iterator iter = linelst.begin(); iter != linelst.end(); ++iter){
		line = trim((*iter));
		if(0 == line.length()){
			continue;
		}

		// check section keywords
		if(INICFG_COMMENT_CHAR == line[0]){
			continue;
		}else if(line == INICFG_K2HTPDTOR_SEC_STR){
			is_in_section = true;
			continue;
		}else if(INICFG_SEC_START_CHAR == line[0] && INICFG_SEC_END_CHAR == line[line.length() - 1]){
			// other section start
			is_in_section = false;
			continue;
		}
		if(!is_in_section){
			continue;
		}

		// in section -> parse key and value
		string				value("");
		string::size_type	pos;
		if(string::npos != (pos = line.find(INICFG_KV_SEP))){
			value	= trim(line.substr(pos + 1));
			line	= upper(trim(line.substr(0, pos)));
		}

		// check and set
		if(0 != value.length()){
			// find key name
			if(line == INICFG_K2HTPDTOR_BC_STR){
				value	= upper(value);
				if(value == INICFG_K2HTPDTOR_VAL_ON || value == INICFG_K2HTPDTOR_VAL_YES){
					isbroadcast	= true;
				}else if(value == INICFG_K2HTPDTOR_VAL_OFF || value == INICFG_K2HTPDTOR_VAL_NO){
					isbroadcast	= false;
				}else{
					WAN_K2HPRN("Unknown value(%s) for %s key, so skip this line.", value.c_str(), line.c_str());
				}

			}else if(line == INICFG_K2HTPDTOR_CHMPXCONF_STR){
				chmpxconf = value;

			}else if(line == INICFG_K2HTPDTOR_EXCEPTKEY_STR){
				unsigned char*	pbytmp;
				size_t			length = 0;
				if(NULL != (pbytmp = cvt_escape_sequence(value.c_str(), length, false))){
					PK2HTPBINBUF	pbyptr = new K2HTPBINBUF;
					pbyptr->bydata = pbytmp;
					pbyptr->length = length;
					excepts.push_back(pbyptr);
				}

			}else if(line == INICFG_K2HTPDTOR_FILE_STR){
				// check & set file path
				if(!make_exist_file(value.c_str(), outputfile)){
					WAN_K2HPRN("could not make the file path(%s) by %s key.", value.c_str(), line.c_str());
					outputfile.erase();
				}

			}else if(line == INICFG_K2HTPDTOR_FILTER_STR){
				// set except types
				if(!parse_except_types(value.c_str(), excepttypes)){
					WAN_K2HPRN("could not load except transaction type(%s) by %s key.", value.c_str(), line.c_str());
					excepttypes.clear();
				}
			}else{
				// unknown key name.
				MSG_K2HPRN("Unknown key(%s), so skip this line.", line.c_str());
			}
		}else{
			WAN_K2HPRN("The value is empty for %s key, so skip this line.", line.c_str());
		}
	}
	return true;
}

bool K2HtpDtorManager::LoadConfigrationYaml(const char* config, string& chmpxconf, bool& isbroadcast, dtorpbylst_t& excepts, string& outputfile, excepttypemap_t& excepttypes, bool is_json_string)
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
			return false;
		}
		// set file to parser
		yaml_parser_set_input_file(&yparser, fp);

	}else{	// JSON_STR
		// set string to parser
		yaml_parser_set_input_string(&yparser, reinterpret_cast<const unsigned char*>(config), strlen(config));
	}

	// Do parsing
	chmpxconf		= config;			// default
	isbroadcast		= false;			// default
	outputfile		= "";				// default
	bool	result	= K2HtpDtorManager::LoadConfigrationYamlTopLevel(yparser, chmpxconf, isbroadcast, excepts, outputfile, excepttypes);

	yaml_parser_delete(&yparser);
	if(fp){
		fclose(fp);
	}
	return result;
}

bool K2HtpDtorManager::LoadConfigrationYamlTopLevel(yaml_parser_t& yparser, string& chmpxconf, bool& isbroadcast, dtorpbylst_t& excepts, string& outputfile, excepttypemap_t& excepttypes)
{
	CHMYamlDataStack	other_stack;
	bool				is_set_dtor	= false;
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
					// Found Top Level Keywards, start to loading
					if(0 == strcasecmp(CFG_K2HTPDTOR_SEC_STR, reinterpret_cast<const char*>(yevent.data.scalar.value))){
						if(is_set_dtor){
							MSG_K2HPRN("Got yaml scalar event in loop, but already loading %s. Thus stacks this event.", CFG_K2HTPDTOR_SEC_STR);
							if(!other_stack.add(yevent.type)){
								result = false;
							}
						}else{
							// Load K2HTPDTOR section
							if(!K2HtpDtorManager::LoadConfigrationYamlContents(yparser, chmpxconf, isbroadcast, excepts, outputfile, excepttypes)){
								ERR_K2HPRN("Something error occured in loading %s section.", CFG_K2HTPDTOR_SEC_STR);
								result = false;
							}
						}

					}else{
						MSG_K2HPRN("Got yaml scalar event in loop, but unknown keyward(%s) for me. Thus stacks this event.", reinterpret_cast<const char*>(yevent.data.scalar.value));
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

bool K2HtpDtorManager::LoadConfigrationYamlContents(yaml_parser_t& yparser, string& chmpxconf, bool& isbroadcast, dtorpbylst_t& excepts, string& outputfile, excepttypemap_t& excepttypes)
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
				if(0 == strcasecmp(INICFG_K2HTPDTOR_BC_STR, key.c_str())){
					string value	= upper(string(reinterpret_cast<const char*>(yevent.data.scalar.value)));
					if(value == INICFG_K2HTPDTOR_VAL_ON || value == INICFG_K2HTPDTOR_VAL_YES){
						isbroadcast	= true;
					}else if(value == INICFG_K2HTPDTOR_VAL_OFF || value == INICFG_K2HTPDTOR_VAL_NO){
						isbroadcast	= false;
					}else{
						WAN_K2HPRN("Unknown value(%s) for %s key, so skip this line.", value.c_str(), key.c_str());
					}

				}else if(0 == strcasecmp(INICFG_K2HTPDTOR_CHMPXCONF_STR, key.c_str())){
					chmpxconf = reinterpret_cast<const char*>(yevent.data.scalar.value);

				}else if(0 == strcasecmp(INICFG_K2HTPDTOR_EXCEPTKEY_STR, key.c_str())){
					unsigned char*	pbytmp;
					size_t			length = 0;
					if(NULL != (pbytmp = cvt_escape_sequence(reinterpret_cast<const char*>(yevent.data.scalar.value), length, false))){
						PK2HTPBINBUF	pbyptr = new K2HTPBINBUF;
						pbyptr->bydata = pbytmp;
						pbyptr->length = length;
						excepts.push_back(pbyptr);
					}

				}else if(0 == strcasecmp(INICFG_K2HTPDTOR_FILE_STR, key.c_str())){
					// check & set file path
					if(!make_exist_file(reinterpret_cast<const char*>(yevent.data.scalar.value), outputfile)){
						WAN_K2HPRN("could not make the file path(%s) by %s key.", reinterpret_cast<const char*>(yevent.data.scalar.value), key.c_str());
						outputfile.erase();
					}

				}else if(0 == strcasecmp(INICFG_K2HTPDTOR_FILTER_STR, key.c_str())){
					// set except types
					if(!parse_except_types(reinterpret_cast<const char*>(yevent.data.scalar.value), excepttypes)){
						WAN_K2HPRN("could not load except transaction type(%s) by %s key.", reinterpret_cast<const char*>(yevent.data.scalar.value), key.c_str());
						excepttypes.clear();
					}

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
			ERR_K2HPRN("Found unexpected yaml event(%d) in %s section.", yevent.type, CFG_K2HTPDTOR_SEC_STR);
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

//---------------------------------------------------------
// K2HtpDtorManager : Methods
//---------------------------------------------------------
K2HtpDtorManager::K2HtpDtorManager() : LockVal(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED), run_thread(false), epollfd(DTOR_INVALID_HANDLE)
{
	dtormap.clear();
	dtorlist.clear();

	// create epoll fd
	if(DTOR_INVALID_HANDLE == (epollfd = epoll_create1(EPOLL_CLOEXEC))){
		ERR_K2HPRN("Could not make epoll fd by errno(%d), but continue,,,", errno);
	}else{
		// run watch thread
		int	result;
		run_thread = true;
		if(0 != (result = pthread_create(&watch_thread, NULL, K2HtpDtorManager::WorkerThread, this))){
			ERR_K2HPRN("Could not create thread by errno(%d), but continue,,,", result);
			run_thread = false;
		}
	}
}

K2HtpDtorManager::~K2HtpDtorManager()
{
	// stop watch thread
	if(run_thread){
		run_thread = false;

		// wait for thread exit
		void*	pretval = NULL;
		int		result;
		if(0 != (result = pthread_join(watch_thread, &pretval))){
			ERR_K2HPRN("Failed to wait thread exiting by errno(%d), but continue,,,", result);
		}else{
			MSG_K2HPRN("Succeed to wait thread exiting,");
		}
	}

	// set remove flag for all dtor
	fullock::flck_lock_noshared_mutex(&LockVal);				// MUTEX LOCK
	for(dtormap_t::iterator iter = dtormap.begin(); iter != dtormap.end(); dtormap.erase(iter++)){
		PDTORINFO	pdtor	= iter->second;
		iter->second		= NULL;
		if(!pdtor){
			ERR_K2HPRN("K2HTPDTOR k2hash handle(0x%016" PRIx64 ") does not have DTOR information, but continue...", iter->first);
		}else{
			// set remove flag
			pdtor->remove_this = true;
		}
	}
	fullock::flck_unlock_noshared_mutex(&LockVal);				// MUTEX UNLOCK

	// remove dtor manually
	if(!UpdateWatch(true)){
		WAN_K2HPRN("Somthing wrong to remove all dtor list.");
	}

	// close epoll
	K2H_CLOSE(epollfd);
}

bool K2HtpDtorManager::Add(k2h_h k2hhandle, const char* pchmpxconf)
{
	if(K2H_INVALID_HANDLE == k2hhandle || ISEMPTYSTR(pchmpxconf)){
		ERR_K2HPRN("Paraemters are wrong.");
		return false;
	}

	// check conf file
	string			chmpxconf("");
	string			outputfile("");
	bool			isbroadcast = false;
	dtorpbylst_t	excepts;
	excepttypemap_t	excepttypes;
	if(!K2HtpDtorManager::LoadConfigration(pchmpxconf, chmpxconf, isbroadcast, excepts, outputfile, excepttypes)){
		ERR_K2HPRN("Configuration file(%s) has something wrong.", pchmpxconf);
		return false;
	}

	// Duplicate handle check
	while(!fullock::flck_trylock_noshared_mutex(&LockVal));			// MUTEX LOCK
	if(dtormap.end() != dtormap.find(k2hhandle)){
		// already has k2hhandle mapping but it is not same conf file, so remove this(in thread).
		PDTORINFO	pdtor = dtormap[k2hhandle];
		if(pdtor){
			pdtor->remove_this = true;
		}
		dtormap.erase(k2hhandle);
	}
	fullock::flck_unlock_noshared_mutex(&LockVal);					// MUTEX UNLOCK

	// create new chmpx handle.
	PDTORINFO	pdtor = new DTORINFO;
	if(CHM_INVALID_CHMPXHANDLE == (pdtor->chmpxhandle = chmpx_create(chmpxconf.c_str(), false, true))){
		ERR_K2HPRN("K2HTPDTOR could not open chmpx handle for k2hash handle(0x%016" PRIx64 ") and chmpxconf(%s).", k2hhandle, chmpxconf.c_str());
		K2H_Delete(pdtor);
		return false;
	}
	pdtor->is_broadcast	= isbroadcast;
	pdtor->chmpxconf	= pchmpxconf;
	pdtor->outputfile	= outputfile;
	pdtor->excepts.swap(excepts);
	pdtor->excepttypes.swap(excepttypes);

	// set new handle
	while(!fullock::flck_trylock_noshared_mutex(&LockVal));			// MUTEX LOCK
	dtorlist.push_back(pdtor);
	dtormap[k2hhandle]	= pdtor;
	fullock::flck_unlock_noshared_mutex(&LockVal);					// MUTEX UNLOCK

	return true;
}

bool K2HtpDtorManager::Remove(k2h_h k2hhandle)
{
	if(K2H_INVALID_HANDLE == k2hhandle){
		ERR_K2HPRN("Paraemter is wrong.");
		return false;
	}

	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// MUTEX LOCK

	if(dtormap.end() == dtormap.find(k2hhandle)){
		// There is no k2hhandle.
		WAN_K2HPRN("K2HTPDTOR already does not have k2hash handle(0x%016" PRIx64 ").", k2hhandle);
	}else{
		PDTORINFO	pdtor	= dtormap[k2hhandle];
		pdtor->remove_this	= true;
		if(pdtor){
			pdtor->remove_this	= true;
		}
		dtormap.erase(k2hhandle);
	}
	fullock::flck_unlock_noshared_mutex(&LockVal);				// MUTEX UNLOCK

	return true;
}

bool K2HtpDtorManager::UpdateWatch(bool force)
{
	if(!force && !run_thread){
		ERR_K2HPRN("Watch thread does not run now.");
		return false;
	}
	if(DTOR_INVALID_HANDLE == epollfd){
		ERR_K2HPRN("epoll fd is not initialized.");
		return false;
	}

	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// MUTEX LOCK

	// [NOTE]
	// When dtor remove(delete), call chmpx dectructor.
	// Thus it call k2hash for chmpx dtor removing, and reentrant and dead locking.
	// So we removing dtor after unlock mutex.
	//
	dtorlist_t	remove_dtors;

	// check all dtor
	for(dtorlist_t::iterator iter = dtorlist.begin(); iter != dtorlist.end(); ){
		PDTORINFO	pdtor = *iter;
		if(!pdtor){
			// why?
			iter = dtorlist.erase(iter);
			continue;
		}

		if(pdtor->remove_this){
			// remove this dtor
			if(DTOR_INVALID_HANDLE != pdtor->inotify_fd || DTOR_INVALID_HANDLE != pdtor->watch_fd){
				// remove inotify fd from epoll
				if(-1 == epoll_ctl(epollfd, EPOLL_CTL_DEL, pdtor->inotify_fd, NULL)){
					ERR_K2HPRN("Fialed to remove inotify fd(%d) from epoll fd(%d) by errno(%d), but continue...", pdtor->inotify_fd, epollfd, errno);
				}
				if(-1 == inotify_rm_watch(pdtor->inotify_fd, pdtor->watch_fd)){
					ERR_K2HPRN("Fialed to remove watch fd(%d) from inotify fd(%d) by errno(%d), but continue...", pdtor->watch_fd, pdtor->inotify_fd, errno);
				}
				FLCK_CLOSE(pdtor->watch_fd);
				FLCK_CLOSE(pdtor->inotify_fd);
			}

			if(DTOR_INVALID_HANDLE != pdtor->outputfd){
				// [NOTE]
				// if writing to fd, it is locked. so we need to lock fd.
				//
				int	result;
				if(0 != (result = fullock_rwlock_wrlock(pdtor->outputfd, 0, 1))){
					ERR_K2HPRN("K2HTPDTOR could not lock for output file(%s) by errno(%d).", pdtor->outputfile.c_str(), result);
				}else{
					if(0 != (result = fullock_rwlock_unlock(pdtor->outputfd, 0, 1))){
						ERR_K2HPRN("K2HTPDTOR could not unlock for output file(%s) by errno(%d).", pdtor->outputfile.c_str(), result);
					}
				}
				K2H_CLOSE(pdtor->outputfd);
			}
			remove_dtors.push_back(pdtor);		// keep remove dtors.

			iter = dtorlist.erase(iter);

		}else if(!pdtor->outputfile.empty()){
			// check file exists
			string	abspath;
			if(!make_exist_file(pdtor->outputfile.c_str(), abspath)){
				WAN_K2HPRN("could not make the file path(%s), so skip it.", pdtor->outputfile.c_str());
			}else{
				// add epoll
				if(DTOR_INVALID_HANDLE == pdtor->inotify_fd || DTOR_INVALID_HANDLE == pdtor->watch_fd){
					// create inotify
					if(DTOR_INVALID_HANDLE == (pdtor->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC))){
						ERR_K2HPRN("Could not create inotify fd for %s by errno(%d).", pdtor->outputfile.c_str(), errno);
					}else{
						// create watch fd
						if(DTOR_INVALID_HANDLE == (pdtor->watch_fd = inotify_add_watch(pdtor->inotify_fd, pdtor->outputfile.c_str(), IN_DELETE | IN_MOVE | IN_DELETE_SELF | IN_MOVE_SELF | IN_ATTRIB))){
							ERR_K2HPRN("Could not add inotify fd(%d) to watch fd for %s by errno(%d).", pdtor->inotify_fd, pdtor->outputfile.c_str(), errno);
							K2H_CLOSE(pdtor->inotify_fd);
						}else{
							// set epoll
							struct epoll_event	epoolev;
							memset(&epoolev, 0, sizeof(struct epoll_event));
							epoolev.data.fd	= pdtor->inotify_fd;
							epoolev.events	= EPOLLIN | EPOLLET;

							if(-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, pdtor->inotify_fd, &epoolev)){
								ERR_K2HPRN("Could not add inotify fd(%d) and watch fd(%d) to epoll fd(%d) for %s by errno(%d).", pdtor->inotify_fd, pdtor->watch_fd, epollfd, pdtor->outputfile.c_str(), errno);
								if(-1 == inotify_rm_watch(pdtor->inotify_fd, pdtor->watch_fd)){
									ERR_K2HPRN("Fialed to remove watch fd(%d) from inotify fd(%d) by errno(%d), but continue...", pdtor->watch_fd, pdtor->inotify_fd, errno);
								}
								K2H_CLOSE(pdtor->watch_fd);
								K2H_CLOSE(pdtor->inotify_fd);
							}else{
								// succeed
							}
						}
					}
				}else{
					// already to add inotify id to epoll.
				}
			}
			++iter;
		}else{
			// not need to add watch because pdtor->outputfile is empty
			++iter;
		}
	}
	fullock::flck_unlock_noshared_mutex(&LockVal);				// MUTEX UNLOCK

	// deleting dtor
	for(dtorlist_t::iterator iter = remove_dtors.begin(); iter != remove_dtors.end(); ++iter){
		PDTORINFO	pdtor = *iter;
		K2H_Delete(pdtor);					// with detach chmpx
	}
	return true;
}

bool K2HtpDtorManager::CheckWatch(int inotifyfd)
{
	// Loop for checking event
	bool	is_file_changed = false;
	while(!is_file_changed){
		// [NOTICE]
		// Some systems cannot read integer variables if they are not properly aligned.
		// On other systems, incorrect alignment may decrease performance.
		// Hence, the buffer used for reading from the inotify file descriptor should 
		// have the same alignment as struct inotify_event.
		//
		unsigned char	buff[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
		ssize_t			length;
		memset(buff, 0, sizeof(buff));

		// read from inotify event[s]
		if(-1 == (length = read(inotifyfd, buff, sizeof(buff)))){
			if(EAGAIN == errno){
				//MSG_K2HPRN("reach to EAGAIN.");
			}else{
				WAN_K2HPRN("errno(%d) occurred during reading from inotify fd(%d)", errno, inotifyfd);
			}
			break;
		}
		if(0 >= length){
			//MSG_K2HPRN("no more inotify event data.");
			break;
		}

		// loop by each inotify_event
		const struct inotify_event*	in_event;
		for(unsigned char* ptr = buff; !is_file_changed && ptr < (buff + length); ptr += sizeof(struct inotify_event) + in_event->len){
			in_event = (const struct inotify_event*)ptr;

			if(in_event->mask & IN_DELETE || in_event->mask & IN_DELETE_SELF || in_event->mask & IN_MOVE_SELF || in_event->mask & IN_MOVED_FROM || in_event->mask & IN_MOVED_TO){
				// remove or move the file
				is_file_changed = true;
			}else if(in_event->mask & IN_ATTRIB){
				// check file exists
				while(!fullock::flck_trylock_noshared_mutex(&LockVal));	// MUTEX LOCK

				for(dtorlist_t::iterator iter = dtorlist.begin(); iter != dtorlist.end(); ++iter){
					PDTORINFO	pdtor = *iter;
					if(pdtor && pdtor->inotify_fd == inotifyfd){
						// check file existing
						struct stat	st;
						if(-1 == stat(pdtor->outputfile.c_str(), &st) || pdtor->output_devid != st.st_dev || pdtor->output_inode != st.st_ino){
							is_file_changed = true;
						}
						break;
					}
				}
				fullock::flck_unlock_noshared_mutex(&LockVal);			// MUTEX UNLOCK
			}
		}
	}
	if(!is_file_changed){
		// nothing to do
		return true;
	}

	//
	// the file is changed, so close opened fd and reset inotify fds
	//
	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// MUTEX LOCK

	// get file path for loop key
	//
	// [NOTE]
	// if there is same output file in dtor list, epoll event is only one shot.
	// so need to check by file path instead of inotify fd.
	//
	string	tgfile;
	for(dtorlist_t::iterator iter = dtorlist.begin(); iter != dtorlist.end(); ++iter){
		PDTORINFO	pdtor = *iter;
		if(pdtor && pdtor->inotify_fd == inotifyfd){
			tgfile = pdtor->outputfile;
			break;
		}
	}
	// loop by file path(remove inotify and close fd)
	for(dtorlist_t::iterator iter = dtorlist.begin(); iter != dtorlist.end(); ++iter){
		PDTORINFO	pdtor = *iter;
		if(pdtor && !pdtor->remove_this && pdtor->outputfile == tgfile){
			// remove inotify fd from epoll
			if(-1 == epoll_ctl(epollfd, EPOLL_CTL_DEL, pdtor->inotify_fd, NULL)){
				ERR_K2HPRN("Fialed to remove inotify fd(%d) from epoll fd(%d) by errno(%d), but continue...", pdtor->inotify_fd, epollfd, errno);
			}
			if(-1 == inotify_rm_watch(pdtor->inotify_fd, pdtor->watch_fd)){
				ERR_K2HPRN("Fialed to remove watch fd(%d) from inotify fd(%d) by errno(%d), but continue...", pdtor->watch_fd, pdtor->inotify_fd, errno);
			}
			FLCK_CLOSE(pdtor->watch_fd);
			FLCK_CLOSE(pdtor->inotify_fd);

			// close fd
			if(DTOR_INVALID_HANDLE != pdtor->outputfd){
				// [NOTE]
				// if writing to fd, it is locked. so we need to lock fd.
				//
				int	result;
				if(0 != (result = fullock_rwlock_wrlock(pdtor->outputfd, 0, 1))){
					ERR_K2HPRN("K2HTPDTOR could not lock for output file(%s) by errno(%d).", pdtor->outputfile.c_str(), result);
				}else{
					if(0 != (result = fullock_rwlock_unlock(pdtor->outputfd, 0, 1))){
						ERR_K2HPRN("K2HTPDTOR could not unlock for output file(%s) by errno(%d).", pdtor->outputfile.c_str(), result);
					}
				}
				K2H_CLOSE(pdtor->outputfd);
			}

			// [NOTE]
			// add inotify at main thread loop
			//
		}
	}
	fullock::flck_unlock_noshared_mutex(&LockVal);				// MUTEX UNLOCK

	return true;
}

bool K2HtpDtorManager::GetChmpxMsgId(k2h_h k2hhandle, chmpx_h& chmpxhandle, msgid_t& msgid, bool& isbroadcast)
{
	if(K2H_INVALID_HANDLE == k2hhandle){
		ERR_K2HPRN("Paraemter is wrong.");
		return false;
	}

	bool	result	= false;
	pid_t	tid		= gettid();				// my thread id
	chmpxhandle		= CHM_INVALID_CHMPXHANDLE;
	msgid			= CHM_INVALID_MSGID;
	isbroadcast		= false;

	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// MUTEX LOCK

	if(dtormap.end() == dtormap.find(k2hhandle)){
		ERR_K2HPRN("K2HTPDTOR does not have k2hash handle(0x%016" PRIx64 ") mapping.", k2hhandle);

	}else{
		PDTORINFO	pdtor	= dtormap[k2hhandle];
		if(pdtor){
			if(pdtor->msgidmap.end() == pdtor->msgidmap.find(tid)){
				MSG_K2HPRN("K2HTPDTOR there is no tid(%d) msgid mapping, so open msgid new.", tid);

				if(CHM_INVALID_MSGID == (msgid = chmpx_open(pdtor->chmpxhandle, false))){
					ERR_K2HPRN("K2HTPDTOR could not open msgid for tid(%d) in chmpx handle(0x%016" PRIx64 "), k2hash handle(0x%016" PRIx64 ").", tid, pdtor->chmpxhandle, k2hhandle);
				}else{
					chmpxhandle				= pdtor->chmpxhandle;
					isbroadcast				= pdtor->is_broadcast;
					pdtor->msgidmap[tid]	= msgid;
					result					= true;
				}
			}else{
				chmpxhandle	= pdtor->chmpxhandle;
				isbroadcast	= pdtor->is_broadcast;
				msgid		= pdtor->msgidmap[tid];
				result		= true;
			}
		}
	}
	fullock::flck_unlock_noshared_mutex(&LockVal);				// MUTEX UNLOCK

	return result;
}

bool K2HtpDtorManager::IsExceptKey(k2h_h k2hhandle, const unsigned char* pkey, size_t length)
{
	if(K2H_INVALID_HANDLE == k2hhandle || ISEMPTYSTR(pkey) || 0 == length){
		ERR_K2HPRN("Paraemters are wrong.");
		return false;
	}

	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// MUTEX LOCK

	bool	result = false;
	if(dtormap.end() == dtormap.find(k2hhandle)){
		ERR_K2HPRN("K2HTPDTOR does not have k2hash handle(0x%016" PRIx64 ") mapping.", k2hhandle);
	}else{
		PDTORINFO	pdtor	= dtormap[k2hhandle];
		if(pdtor){
			result = find_dtorpbylst(pdtor->excepts, pkey, length);
		}
	}
	fullock::flck_unlock_noshared_mutex(&LockVal);				// MUTEX UNLOCK

	return result;
}

bool K2HtpDtorManager::IsExceptType(k2h_h k2hhandle, long type)
{
	if(K2H_INVALID_HANDLE == k2hhandle){
		ERR_K2HPRN("Paraemters are wrong.");
		return false;
	}

	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// MUTEX LOCK

	bool	result = false;
	if(dtormap.end() == dtormap.find(k2hhandle)){
		// There is no k2hhandle.
		WAN_K2HPRN("K2HTPDTOR already does not have k2hash handle(0x%016" PRIx64 ").", k2hhandle);
	}else{
		PDTORINFO	pdtor	= dtormap[k2hhandle];
		if(pdtor){
			result = (pdtor->excepttypes.end() != pdtor->excepttypes.find(type));
		}
	}
	fullock::flck_unlock_noshared_mutex(&LockVal);				// MUTEX UNLOCK

	return result;
}

//
// This method performs the processing of IsExceptKey() and IsExceptType().
//
bool K2HtpDtorManager::IsExcept(k2h_h k2hhandle, const unsigned char* pkey, size_t length, long type)
{
	if(K2H_INVALID_HANDLE == k2hhandle || ISEMPTYSTR(pkey) || 0 == length){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// MUTEX LOCK

	bool	result = false;
	if(dtormap.end() == dtormap.find(k2hhandle)){
		// There is no k2hhandle.
		WAN_K2HPRN("K2HTPDTOR already does not have k2hash handle(0x%016" PRIx64 ").", k2hhandle);
	}else{
		PDTORINFO	pdtor	= dtormap[k2hhandle];
		if(pdtor){
			if(false == (result = (pdtor->excepttypes.end() != pdtor->excepttypes.find(type)))){
				result = find_dtorpbylst(pdtor->excepts, pkey, length);
			}
		}
	}
	fullock::flck_unlock_noshared_mutex(&LockVal);				// MUTEX UNLOCK

	return result;
}

int K2HtpDtorManager::GetOutputFd(k2h_h k2hhandle, bool is_lock)
{
	if(K2H_INVALID_HANDLE == k2hhandle){
		ERR_K2HPRN("Paraemter is wrong.");
		return DTOR_INVALID_HANDLE;
	}

	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// MUTEX LOCK

	if(dtormap.end() == dtormap.find(k2hhandle)){
		ERR_K2HPRN("K2HTPDTOR does not have k2hash handle(0x%016" PRIx64 ") mapping.", k2hhandle);
		fullock::flck_unlock_noshared_mutex(&LockVal);			// MUTEX UNLOCK
		return DTOR_INVALID_HANDLE;
	}

	PDTORINFO	pdtor = dtormap[k2hhandle];
	if(!pdtor || pdtor->outputfile.empty()){
		fullock::flck_unlock_noshared_mutex(&LockVal);			// MUTEX UNLOCK
		return DTOR_INVALID_HANDLE;
	}

	// is opened
	if(DTOR_INVALID_HANDLE != pdtor->outputfd){
		if(is_lock){
			int	result;
			if(0 != (result = fullock_rwlock_wrlock(pdtor->outputfd, 0, 1))){
				ERR_K2HPRN("K2HTPDTOR could not lock for output file(%s) by errno(%d).", pdtor->outputfile.c_str(), result);
				fullock::flck_unlock_noshared_mutex(&LockVal);	// MUTEX UNLOCK
				return DTOR_INVALID_HANDLE;
			}
		}
		fullock::flck_unlock_noshared_mutex(&LockVal);			// MUTEX UNLOCK
		return pdtor->outputfd;
	}

	// check file
	if(DTOR_INVALID_HANDLE == (pdtor->outputfd = get_exist_file(pdtor->outputfile.c_str(), pdtor->output_devid, pdtor->output_inode))){
		ERR_K2HPRN("K2HTPDTOR could not open output file(%s).", pdtor->outputfile.c_str());
		fullock::flck_unlock_noshared_mutex(&LockVal);			// MUTEX UNLOCK
		return DTOR_INVALID_HANDLE;
	}

	// lock
	if(is_lock){
		int	result;
		if(0 != (result = fullock_rwlock_wrlock(pdtor->outputfd, 0, 1))){
			ERR_K2HPRN("K2HTPDTOR could not lock for output file(%s) by errno(%d).", pdtor->outputfile.c_str(), result);
			fullock::flck_unlock_noshared_mutex(&LockVal);		// MUTEX UNLOCK
			return DTOR_INVALID_HANDLE;
		}
	}
	fullock::flck_unlock_noshared_mutex(&LockVal);				// MUTEX UNLOCK

	return pdtor->outputfd;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
