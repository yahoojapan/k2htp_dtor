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
#ifndef	K2HTPDTORMAN_H
#define	K2HTPDTORMAN_H

#include <k2hash/k2hutil.h>
#include <k2hash/k2hdbg.h>
#include <chmpx/chmpx.h>
#include <yaml.h>
#include <string>
#include <map>
#include <list>

#include "k2htpdtorfile.h"

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
// List: for parsing configuration file
//
typedef std::list<std::string>		dtorstrlst_t;


// Vector: for binary array
//
typedef struct k2htp_bin_buffer{
	unsigned char*	bydata;
	size_t			length;

	k2htp_bin_buffer() : bydata(NULL), length(0)
	{
	}
	~k2htp_bin_buffer()
	{
		K2H_Free(bydata);
	}
}K2HTPBINBUF, *PK2HTPBINBUF;

typedef std::list<PK2HTPBINBUF>		dtorpbylst_t;

inline void free_dtorpbylst(dtorpbylst_t& pbylst)
{
	for(dtorpbylst_t::iterator iter = pbylst.begin(); iter != pbylst.end(); pbylst.erase(iter++)){
		K2H_Delete(*iter);
	}
}

inline bool find_dtorpbylst(dtorpbylst_t& pbylst, const unsigned char* pbin, size_t length)
{
	if(!pbin || 0 == length){
		return false;
	}
	// cppcheck-suppress postfixOperator
	for(dtorpbylst_t::const_iterator iter = pbylst.begin(); iter != pbylst.end(); iter++){
		if(length < (*iter)->length){
			continue;
		}
		if(0 == memcmp((*iter)->bydata, pbin, (*iter)->length)){
			return true;
		}
	}
	return false;
}

// Map: Thread ID -> chmpx handle
//
typedef std::map<pid_t, msgid_t>	tidmsgidmap_t;

// Map: except type -> bool(always true)
//
typedef std::map<long, bool>		excepttypemap_t;

// DTORINFO
//
typedef struct dtor_info{
	bool			is_broadcast;
	dtorpbylst_t	excepts;
	excepttypemap_t	excepttypes;			// map for exception transaction type
	std::string		chmpxconf;
	std::string		outputfile;				// output file
	int				outputfd;				// opened fd for output file
	dev_t			output_devid;			// backup device id for opened fd
	ino_t			output_inode;			// backup inode for opened fd
	int				inotify_fd;				// inotify fd for watching file
	int				watch_fd;				// watch fd for epoll
	volatile bool	remove_this;
	chmpx_h			chmpxhandle;
	tidmsgidmap_t	msgidmap;

	dtor_info() : is_broadcast(false), chmpxconf(""), outputfile(""), outputfd(DTOR_INVALID_HANDLE), output_devid(0), output_inode(0), inotify_fd(DTOR_INVALID_HANDLE), watch_fd(DTOR_INVALID_HANDLE), remove_this(false), chmpxhandle(CHM_INVALID_CHMPXHANDLE)
	{
	}
	~dtor_info()
	{
		free_dtorpbylst(excepts);

		for(tidmsgidmap_t::iterator iter = msgidmap.begin(); iter != msgidmap.end(); msgidmap.erase(iter++)){
			if(!chmpx_close(chmpxhandle, iter->second)){
				ERR_K2HPRN("K2HTPDTOR could not close chmpx msgid(0x%016" PRIx64 ") in chmpx handle(0x%016" PRIx64 "), but continue...", iter->second, chmpxhandle);
			}
		}
		msgidmap.clear();
		if(CHM_INVALID_CHMPXHANDLE != chmpxhandle){
			chmpx_destroy(chmpxhandle);
		}
		// [NOTE]
		// Should remove from inotify before closing these.
		//
		K2H_CLOSE(inotify_fd);
		K2H_CLOSE(watch_fd);
		K2H_CLOSE(outputfd);
	}
}DTORINFO, *PDTORINFO;

// Map: K2hash handle -> DTORINFO
//
typedef std::map<k2h_h, PDTORINFO>	dtormap_t;
typedef std::vector<PDTORINFO>		dtorlist_t;

//---------------------------------------------------------
// K2HtpDtorManager Class
//---------------------------------------------------------
class K2HtpDtorManager
{
	protected:
		static const int			SLEEP_MS = 100;	// 100ms default for waiting epoll event
		static const int			WAIT_EVENT_MAX = 32;

		volatile int				LockVal;		// the value to curfwmap(fullock_mutex)
		volatile bool				run_thread;
		int							epollfd;
		pthread_t					watch_thread;
		dtormap_t					dtormap;		// map for k2hash to dtor information.
		dtorlist_t					dtorlist;		// list for dtor information.

	protected:
		static void* WorkerThread(void* param);
		static bool ReadIniFileContents(const char* filepath, dtorstrlst_t& linelst, dtorstrlst_t& allfiles);
		static bool LoadConfiguration(const char* filepath, std::string& chmpxconf, bool& isbroadcast, dtorpbylst_t& excepts, std::string& outputfile, excepttypemap_t& excepttypes);
		static bool LoadConfigurationIni(const char* filepath, std::string& chmpxconf, bool& isbroadcast, dtorpbylst_t& excepts, std::string& outputfile, excepttypemap_t& excepttypes);
		static bool LoadConfigurationYaml(const char* config, std::string& chmpxconf, bool& isbroadcast, dtorpbylst_t& excepts, std::string& outputfile, excepttypemap_t& excepttypes, bool is_json_string);
		static bool LoadConfigurationYamlTopLevel(yaml_parser_t& yparser, std::string& chmpxconf, bool& isbroadcast, dtorpbylst_t& excepts, std::string& outputfile, excepttypemap_t& excepttypes);
		static bool LoadConfigurationYamlContents(yaml_parser_t& yparser, std::string& chmpxconf, bool& isbroadcast, dtorpbylst_t& excepts, std::string& outputfile, excepttypemap_t& excepttypes);

		K2HtpDtorManager();
		virtual ~K2HtpDtorManager();

		bool UpdateWatch(bool force = false);
		bool CheckWatch(int inotifyfd);

	public:
		static K2HtpDtorManager* Get(void);			// singleton object

		bool Add(k2h_h k2hhandle, const char* pchmpxconf);
		bool Remove(k2h_h k2hhandle);
		bool GetChmpxMsgId(k2h_h k2hhandle, chmpx_h& chmpxhandle, msgid_t& msgid, bool& isbroadcast);
		bool IsExceptKey(k2h_h k2hhandle, const unsigned char* pkey, size_t length);
		bool IsExceptType(k2h_h k2hhandle, long type);
		bool IsExcept(k2h_h k2hhandle, const unsigned char* pkey, size_t length, long type);
		int GetOutputFd(k2h_h k2hhandle, bool is_lock);
};

#endif	// K2HTPDTORMAN_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
