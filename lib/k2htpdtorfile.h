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
 * CREATE:   Mon Nov 02 2015
 * REVISION:
 *
 */
#ifndef	K2HTPDTORFILE_H
#define	K2HTPDTORFILE_H

#include <string>

//---------------------------------------------------------
// Symbol
//---------------------------------------------------------
#define	DTOR_INVALID_HANDLE				(-1)

//---------------------------------------------------------
// Utility
//---------------------------------------------------------
bool make_exist_directory(const char* path, std::string& abspath);
bool make_exist_file(const char* path, std::string& abspath);
bool make_exist_file(const char* path);
int get_exist_file(const char* path, dev_t& devid, ino_t& inode);
bool cvt_path_real_path(const char* path, std::string& abspath);
bool k2htpdtor_write(int fd, const unsigned char* data, size_t length);

#endif	// K2HTPDTORFILE_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
