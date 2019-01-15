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
 * CREATE:   Thu Nov 05 2015
 * REVISION:
 *
 */

#ifndef	K2HTPDTORSVRFD_H
#define K2HTPDTORSVRFD_H

#include <string>

//---------------------------------------------------------
// Class K2htpSvrFd
//---------------------------------------------------------
// [NOTE]
// This class is watching file which is removed or moved by inotify.
// So this does not support for the files on network device, special
// drive(/proc, etc), and fuse drive.
// 
// inotify man page:
// Inotify reports only events that a user-space program triggers
// through the filesystem API.  As a result, it does not catch remote
// events that occur on network filesystems.  (Applications must fall
// back to polling the filesystem to catch such events.)  Furthermore,
// various pseudo-filesystems such as /proc, /sys, and /dev/pts are not
// monitorable with inotify.
// 
class K2htpSvrFd
{
	protected:
		static const int	WAIT_EVENT_MAX	= 32;		// wait event max count
		static const int	EPOLL_TIMEOUT_MS= 5;		// 5ms

		volatile int		lockval;					// the value to curfwmap(fullock_mutex)
		volatile bool		run_thread;
		std::string			filepath;
		int					opened_fd;
		dev_t				opened_devid;
		ino_t				opened_inode;
		pthread_t			watch_thread;

	protected:
		static bool AddWatch(int epollfd, const std::string& filepath, int& inotify_fd, int& watch_fd);
		static bool DeleteWatch(int epollfd, int& inotify_fd, int& watch_fd);
		static bool CheckInotifyEvents(int inotifyfd, K2htpSvrFd* pSvrFdObj);
		static void* WorkerThread(void* param);

		bool Close(void);
		bool Open(const char* path);
		bool Open(bool is_locked);
		bool RunThread(void);
		bool StopThread(void);

	public:
		K2htpSvrFd(void);
		virtual ~K2htpSvrFd(void);

		bool Initialize(const char* path);
		bool Write(const unsigned char* pdata, size_t length);
};

#endif	// K2HTPDTORSVRFD_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
