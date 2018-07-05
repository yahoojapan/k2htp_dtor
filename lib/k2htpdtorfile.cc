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
 * CREATE:   Mon Nov 02 2015
 * REVISION:
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <k2hash/k2hash.h>
#include <k2hash/k2hutil.h>
#include <k2hash/k2hdbg.h>

#include "k2htpdtorfile.h"

using namespace std;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	K2HDTOR_DIR_PERM			(S_IRWXU | S_IRWXG | S_IRWXO)
#define	K2HDTOR_FILE_PERM			(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

//---------------------------------------------------------
// Utility
//---------------------------------------------------------
bool make_exist_directory(const char* path, string& abspath)
{
	if(ISEMPTYSTR(path)){
		ERR_K2HPRN("path is NULL.");
		return false;
	}

	struct stat	st;
	char*		rpath;
	if(0 == stat(path, &st)){
		// file already exists
		if(!S_ISDIR(st.st_mode)){
			ERR_K2HPRN("path(%s) already exists, but it is not directory.", path);
			return false;
		}
		if(NULL == (rpath = realpath(path, NULL))){
			MSG_K2HPRN("Could not convert path(%s) to real path or there is no such file(errno:%d).", path, errno);
			return false;
		}
		abspath = rpath;
		K2H_Free(rpath);
		return true;
	}

	// check for upper directory
	string				tmppath	= path;
	string::size_type	pos		= tmppath.find_last_of('/');
	if(string::npos == pos){
		ERR_K2HPRN("Not found \"/\" separator in path(%s).", path);
		return false;
	}
	if(0 == pos){
		ERR_K2HPRN("there is no more upper directory for path(%s).", path);
		return false;
	}

	// make upper directory(reentrant)
	tmppath	= tmppath.substr(0, pos);
	if(!make_exist_directory(tmppath.c_str(), abspath)){
		return false;
	}

	// recheck real path
	tmppath	= path;
	pos		= tmppath.find_last_of('/');
	abspath += '/';
	abspath += tmppath.substr(pos + 1);
	if(NULL != (rpath = realpath(abspath.c_str(), NULL))){
		abspath = rpath;
		K2H_Free(rpath);
		return true;
	}

	// make directory
	if(0 != mkdir(abspath.c_str(), K2HDTOR_DIR_PERM)){
		ERR_K2HPRN("could not make directory path(%s) by error(%d).", abspath.c_str(), errno);
		return false;
	}

	// recheck real path
	if(NULL == (rpath = realpath(abspath.c_str(), NULL))){
		MSG_K2HPRN("Could not convert path(%s) to real path or there is no such file(errno:%d).", abspath.c_str(), errno);
		return false;
	}
	abspath = rpath;
	K2H_Free(rpath);

	return true;
}

static bool make_exist_file_ex(const char* path, string& abspath, int& fd, dev_t& devid, ino_t& inode, bool is_open)
{
	if(ISEMPTYSTR(path)){
		ERR_K2HPRN("path is NULL.");
		return false;
	}
	struct stat	st;
	char*		rpath;
	if(0 == stat(path, &st)){
		// file already exists
		if(!S_ISREG(st.st_mode)){
			ERR_K2HPRN("path(%s) already exists, but it is not regular file.", path);
			return false;
		}
		if(NULL == (rpath = realpath(path, NULL))){
			MSG_K2HPRN("Could not convert path(%s) to real path or there is no such file(errno:%d).", path, errno);
			return false;
		}
		abspath = rpath;
		K2H_Free(rpath);

		if(is_open){
			if(DTOR_INVALID_HANDLE == (fd = open(abspath.c_str(), O_CREAT | O_RDWR, K2HDTOR_FILE_PERM))){
				ERR_K2HPRN("could not make file path(%s).", abspath.c_str());
				return false;
			}
			devid = st.st_dev;
			inode = st.st_ino;
		}
		return true;
	}

	// check for upper directory
	string				tmppath	= path;
	string::size_type	pos		= tmppath.find_last_of('/');
	if(string::npos == pos){
		ERR_K2HPRN("Not found \"/\" separator in path(%s).", path);
		return false;
	}
	if(0 == pos){
		ERR_K2HPRN("there is no more upper directory for path(%s).", path);
		return false;
	}
	tmppath	= tmppath.substr(0, pos);
	if(!make_exist_directory(tmppath.c_str(), abspath)){
		return false;
	}

	// recheck real path exists
	tmppath	= path;
	pos		= tmppath.find_last_of('/');
	abspath += '/';
	abspath += tmppath.substr(pos + 1);
	if(NULL != (rpath = realpath(abspath.c_str(), NULL))){
		if(0 != stat(rpath, &st)){
			ERR_K2HPRN("path(%s) doe not exist after success to get real path.", rpath);
			K2H_Free(rpath);
			return false;
		}
		if(!S_ISREG(st.st_mode)){
			ERR_K2HPRN("path(%s) already exists, but it is not regular file.", rpath);
			K2H_Free(rpath);
			return false;
		}
		abspath = rpath;
		K2H_Free(rpath);

		if(is_open){
			if(DTOR_INVALID_HANDLE == (fd = open(abspath.c_str(), O_CREAT | O_RDWR, K2HDTOR_FILE_PERM))){
				ERR_K2HPRN("could not make file path(%s).", abspath.c_str());
				return false;
			}
			devid = st.st_dev;
			inode = st.st_ino;
		}
		return true;
	}

	// make file
	if(DTOR_INVALID_HANDLE == (fd = open(abspath.c_str(), O_CREAT | O_RDWR, K2HDTOR_FILE_PERM))){
		ERR_K2HPRN("could not make file path(%s) by errno(%d).", abspath.c_str(), errno);
		return false;
	}

	// recheck real path
	if(NULL == (rpath = realpath(abspath.c_str(), NULL))){
		MSG_K2HPRN("Could not convert path(%s) to real path or there is no such file(errno:%d).", abspath.c_str(), errno);
		K2H_CLOSE(fd);
		return false;
	}
	abspath = rpath;
	K2H_Free(rpath);

	if(is_open){
		if(0 != stat(abspath.c_str(), &st)){
			ERR_K2HPRN("path(%s) doe not exist after success to get real path.", abspath.c_str());
			K2H_CLOSE(fd);
			return false;
		}
		devid = st.st_dev;
		inode = st.st_ino;
	}else{
		K2H_CLOSE(fd);
	}
	return true;
}

bool make_exist_file(const char* path, string& abspath)
{
	int		fd		= DTOR_INVALID_HANDLE;
	dev_t	devid	= 0;
	ino_t	inode	= 0;
	return make_exist_file_ex(path, abspath, fd, devid, inode, false);
}

bool make_exist_file(const char* path)
{
	string	abspath;
	int		fd		= DTOR_INVALID_HANDLE;
	dev_t	devid	= 0;
	ino_t	inode	= 0;
	return make_exist_file_ex(path, abspath, fd, devid, inode, false);
}

int get_exist_file(const char* path, dev_t& devid, ino_t& inode)
{
	string	abspath("");
	int		fd	= DTOR_INVALID_HANDLE;
	devid		= 0;
	inode		= 0;
	if(!make_exist_file_ex(path, abspath, fd, devid, inode, true)){
		return DTOR_INVALID_HANDLE;
	}
	return fd;
}

bool cvt_path_real_path(const char* path, string& abspath)
{
	if(ISEMPTYSTR(path)){
		ERR_K2HPRN("path is empty.");
		return false;
	}
	char*	rpath;
	if(NULL == (rpath = realpath(path, NULL))){
		MSG_K2HPRN("Could not convert path(%s) to real path or there is no such file(errno:%d).", path, errno);
		return false;
	}
	abspath = rpath;
	K2H_Free(rpath);
	return true;
}

bool k2htpdtor_write(int fd, const unsigned char* data, size_t length)
{
	for(ssize_t wrote = 0, onewrote = 0; static_cast<size_t>(wrote) < length; wrote += onewrote){
		if(-1 == (onewrote = write(fd, &data[wrote], (length - wrote)))){
			WAN_K2HPRN("Failed to write data(%p) length(%zu) to fd(%d) by errno(%d).", &data[wrote], (length - wrote), fd, errno);
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
