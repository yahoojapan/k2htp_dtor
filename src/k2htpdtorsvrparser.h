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
 * CREATE:   Wed Nov 04 2015
 * REVISION:
 *
 */

#ifndef	K2HTPDTORSVRPARSER_H
#define K2HTPDTORSVRPARSER_H

#include <k2hash/k2hshm.h>
#include <k2hash/k2htrans.h>
#include <string>

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
typedef struct k2htpdtorsvr_info{
	std::string		conffile;
	std::string		output_file;
	std::string		plugin_param;

	bool			is_k2hash;
	bool			k2hash_type_mem;
	bool			k2hash_type_tmp;
	std::string		k2hash_file;
	bool			k2hash_fullmap;
	bool			k2hash_init;
	int				k2hash_mask_bitcnt;
	int				k2hash_cmask_bitcnt;
	int				k2hash_max_element_cnt;
	size_t			k2hash_pagesize;

	std::string		trans_ini_file;
	int				trans_dtor_threads;

	k2htpdtorsvr_info(void) :	conffile(""), output_file(""), plugin_param(""), is_k2hash(false), k2hash_type_mem(false), k2hash_type_tmp(false),
								k2hash_file(""), k2hash_fullmap(false), k2hash_init(false), k2hash_mask_bitcnt(K2HShm::DEFAULT_MASK_BITCOUNT),
								k2hash_cmask_bitcnt(K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT), k2hash_max_element_cnt(K2HShm::DEFAULT_MAX_ELEMENT_CNT),
								k2hash_pagesize(K2HShm::MIN_PAGE_SIZE), trans_ini_file(""), trans_dtor_threads(K2HTransManager::DEFAULT_THREAD_POOL)
	{
	}
}K2HTPDTORSVRINFO, *PK2HTPDTORSVRINFO;

//---------------------------------------------------------
// Utilities
//---------------------------------------------------------
bool ParseK2htpdtorsvrConfigration(const char* config, PK2HTPDTORSVRINFO pInfo);

#endif	// K2HTPDTORSVRPARSER_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
