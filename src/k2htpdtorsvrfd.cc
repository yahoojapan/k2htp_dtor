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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <k2hash/k2hash.h>
#include <k2hash/k2hutil.h>
#include <k2hash/k2hdbg.h>
#include <fullock/flckstructure.h>
#include <fullock/flckbaselist.tcc>

#include "k2htpdtorsvrfd.h"
#include "k2htpdtorfile.h"

using namespace std;

//---------------------------------------------------------
// K2htpSvrFd : Class variable
//---------------------------------------------------------
const int	K2htpSvrFd::WAIT_EVENT_MAX;
const int	K2htpSvrFd::EPOLL_TIMEOUT_MS;

//---------------------------------------------------------
// K2htpSvrFd : Class Methods
//---------------------------------------------------------
bool K2htpSvrFd::AddWatch(int epollfd, const string& filepath, int& inotify_fd, int& watch_fd)
{
	if(DTOR_INVALID_HANDLE == epollfd){
		ERR_K2HPRN("parameter is wrong.");
		return false;
	}

	// check file
	if(!make_exist_file(filepath.c_str())){
		ERR_K2HPRN("Could not create exist file(%s).", filepath.c_str());
		return false;
	}

	// create inotify
	if(DTOR_INVALID_HANDLE == (inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC))){
		ERR_K2HPRN("Could not create inotify fd for %s by errno(%d).", filepath.c_str(), errno);
		return false;
	}

	// add watching path
	//
	// [NOTE]
	// IN_DELETE(_SELF) does not occur if the file is removed. :-(
	// So we use IN_ATTRIB instead of it.
	//
	if(DTOR_INVALID_HANDLE == (watch_fd = inotify_add_watch(inotify_fd, filepath.c_str(), IN_DELETE | IN_MOVE | IN_DELETE_SELF | IN_MOVE_SELF | IN_ATTRIB))){
		ERR_K2HPRN("Could not add inotify fd(%d) to watch fd for %s by errno(%d).", inotify_fd, filepath.c_str(), errno);
		K2H_CLOSE(inotify_fd);
		return false;
	}

	// set epoll
	struct epoll_event	epoolev;
	memset(&epoolev, 0, sizeof(struct epoll_event));
	epoolev.data.fd	= inotify_fd;
	epoolev.events	= EPOLLIN | EPOLLET;

	if(-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, inotify_fd, &epoolev)){
		ERR_K2HPRN("Could not add inotify fd(%d) and watch fd(%d) to epoll fd(%d) for %s by errno(%d).", inotify_fd, watch_fd, epollfd, filepath.c_str(), errno);

		if(-1 == inotify_rm_watch(inotify_fd, watch_fd)){
			ERR_K2HPRN("Failed to remove watch fd(%d) from inotify fd(%d) by errno(%d), but continue...", watch_fd, inotify_fd, errno);
		}
		K2H_CLOSE(watch_fd);
		K2H_CLOSE(inotify_fd);
		return false;
	}
	MSG_K2HPRN("Succeed adding file(%s) watch to epoll.", filepath.c_str());

	return true;
}

bool K2htpSvrFd::DeleteWatch(int epollfd, int& inotify_fd, int& watch_fd)
{
	if(DTOR_INVALID_HANDLE == epollfd){
		ERR_K2HPRN("parameter is wrong.");
		return false;
	}

	if(DTOR_INVALID_HANDLE != inotify_fd){
		if(-1 == epoll_ctl(epollfd, EPOLL_CTL_DEL, inotify_fd, NULL)){
			ERR_K2HPRN("Failed to remove inotify fd(%d) from epoll fd(%d) by errno(%d), but continue...", inotify_fd, epollfd, errno);
		}
	}
	if(DTOR_INVALID_HANDLE != inotify_fd && DTOR_INVALID_HANDLE != watch_fd){
		if(-1 == inotify_rm_watch(inotify_fd, watch_fd)){
			ERR_K2HPRN("Failed to remove watch fd(%d) from inotify fd(%d) by errno(%d), but continue...", watch_fd, inotify_fd, errno);
		}
	}
	K2H_CLOSE(watch_fd);
	K2H_CLOSE(inotify_fd);

	MSG_K2HPRN("Succeed deleting file watch from epoll.");

	return true;
}

//
// If move and remove event, returns true.
//
bool K2htpSvrFd::CheckInotifyEvents(int inotifyfd, K2htpSvrFd* pSvrFdObj)
{
	if(!pSvrFdObj){
		ERR_K2HPRN("parameter is wrong");
		return false;
	}

	// Loop
	for(;;){
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
				return false;
			}
			WAN_K2HPRN("errno(%d) occurred during reading from inotify fd(%d)", errno, inotifyfd);
			return false;
		}
		if(0 >= length){
			//MSG_K2HPRN("no more inotify event data.");
			return false;
		}

		// loop by each inotify_event
		const struct inotify_event*	in_event;
		for(unsigned char* ptr = buff; ptr < (buff + length); ptr += sizeof(struct inotify_event) + in_event->len){
			in_event = (const struct inotify_event*)ptr;

			if(in_event->mask & IN_DELETE || in_event->mask & IN_DELETE_SELF || in_event->mask & IN_MOVE_SELF || in_event->mask & IN_MOVED_FROM || in_event->mask & IN_MOVED_TO){
				// remove or move the file
				return true;
			}else if(in_event->mask & IN_ATTRIB){
				// check file exists
				struct stat	st;
				if(-1 == stat(pSvrFdObj->filepath.c_str(), &st)){
					MSG_K2HPRN("Could not get stat for %s(errno:%d), so the file is removed or moved.", pSvrFdObj->filepath.c_str(), errno);
					return true;
				}

				// [NOTE]
				// we do not lock here for performance.
				//
				if(pSvrFdObj->opened_devid != st.st_dev || pSvrFdObj->opened_inode != st.st_ino){
					MSG_K2HPRN("devid(%lu)/inodeid(%lu) for %s is not same as in cache( devid(%lu)/inodeid(%lu) )", st.st_dev, st.st_ino, pSvrFdObj->filepath.c_str(), pSvrFdObj->opened_devid, pSvrFdObj->opened_inode);
					return true;
				}
			}
		}
	}
	return false;
}

void* K2htpSvrFd::WorkerThread(void* param)
{
	K2htpSvrFd*	pSvrFdObj = reinterpret_cast<K2htpSvrFd*>(param);
	if(!pSvrFdObj){
		ERR_K2HPRN("parameter is wrong.");
		pthread_exit(NULL);
	}

	// create epoll and set watch file
	int	epollfd;
	if(DTOR_INVALID_HANDLE == (epollfd = epoll_create1(EPOLL_CLOEXEC))){
		ERR_K2HPRN("Could not make epoll fd by errno(%d).", errno);
		pthread_exit(NULL);
	}

	// watch loop
	struct timespec	sleeptime	= {0L, 1 * 1000 * 1000};	// 1ms
	int				inotify_fd	= DTOR_INVALID_HANDLE;
	int				watch_fd	= DTOR_INVALID_HANDLE;
	while(pSvrFdObj->run_thread){
		// add watch file
		while(DTOR_INVALID_HANDLE == inotify_fd && pSvrFdObj->run_thread){
			if(!K2htpSvrFd::AddWatch(epollfd, pSvrFdObj->filepath, inotify_fd, watch_fd)){
				ERR_K2HPRN("Could not set watch file(%s) to epoll, so retry after waiting a while", pSvrFdObj->filepath.c_str());
			}
			nanosleep(&sleeptime, NULL);
		}
		if(!pSvrFdObj->run_thread){
			break;
		}

		// wait events
		struct epoll_event  events[WAIT_EVENT_MAX];
		int					eventcnt;
		if(0 < (eventcnt = epoll_pwait(epollfd, events, WAIT_EVENT_MAX, K2htpSvrFd::EPOLL_TIMEOUT_MS, NULL))){		// 5ms waiting(default)
			// catch event
			for(int cnt = 0; cnt < eventcnt; cnt++){
				// check removing/moving file
				if(K2htpSvrFd::CheckInotifyEvents(events[cnt].data.fd, pSvrFdObj)){
					// close fd
					if(!pSvrFdObj->Close()){
						ERR_K2HPRN("Failed to close file(%s), but continue...", pSvrFdObj->filepath.c_str());
					}
					// delete watch file
					if(!K2htpSvrFd::DeleteWatch(epollfd, inotify_fd, watch_fd)){
						ERR_K2HPRN("Failed to remove watch file(%s) from epoll, but continue...", pSvrFdObj->filepath.c_str());
					}
				}
			}

		}else if(-1 >= eventcnt){
			if(EINTR != errno){
				ERR_K2HPRN("Something error occured in waiting event fd(%d) by errno(%d)", epollfd, errno);
				break;
			}
			// signal occurred.

		}else{	// 0 == eventcnt
			// timeouted, nothing to do
		}
	}

	// remove watch file and close epoll
	if(DTOR_INVALID_HANDLE != inotify_fd){
		if(!K2htpSvrFd::DeleteWatch(epollfd, inotify_fd, watch_fd)){
			ERR_K2HPRN("Failed to remove watch file(%s) from epoll, but continue...", pSvrFdObj->filepath.c_str());
		}else{
			MSG_K2HPRN("Succeed deleting file(%s) watch and break loop to exiting thread.", pSvrFdObj->filepath.c_str());
		}
	}
	K2H_CLOSE(epollfd);

	pthread_exit(NULL);
	return NULL;
}

//---------------------------------------------------------
// K2htpSvrFd : Methods
//---------------------------------------------------------
K2htpSvrFd::K2htpSvrFd(void) : lockval(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED), run_thread(false), filepath(""), opened_fd(DTOR_INVALID_HANDLE), opened_devid(0), opened_inode(0)
{
}

K2htpSvrFd::~K2htpSvrFd(void)
{
	// stop watch thread
	if(!StopThread()){
		ERR_K2HPRN("Could not stop thread, but continue...");
	}
	// close file
	if(!Close()){
		ERR_K2HPRN("Could not close file, but continue...");
	}
}

bool K2htpSvrFd::Initialize(const char* path)
{
	// open file
	if(!Close() || !Open(path)){
		ERR_K2HPRN("Could not initialize about file(%s).", path);
		return false;
	}
	// run watch thread
	if(!RunThread()){
		ERR_K2HPRN("Could not run thread.");
		return false;
	}
	return true;
}

bool K2htpSvrFd::Close(void)
{
	while(!fullock::flck_trylock_noshared_mutex(&lockval));		// MUTEX LOCK

	if(DTOR_INVALID_HANDLE != opened_fd){
		K2H_CLOSE(opened_fd);
	}
	opened_devid = 0;
	opened_inode = 0;

	fullock::flck_unlock_noshared_mutex(&lockval);				// MUTEX UNLOCK

	return true;
}

bool K2htpSvrFd::Open(const char* path)
{
	while(!fullock::flck_trylock_noshared_mutex(&lockval));		// MUTEX LOCK

	if(!make_exist_file(path, filepath)){
		ERR_K2HPRN("Could not open file(%s).", path);
		fullock::flck_unlock_noshared_mutex(&lockval);			// MUTEX UNLOCK
		return false;
	}
	return Open(true);
}

bool K2htpSvrFd::Open(bool is_locked)
{
	// [NOTE]
	// for performance, so we do not lock here.
	//
	if(DTOR_INVALID_HANDLE != opened_fd){
		return true;
	}
	if(!is_locked){
		while(!fullock::flck_trylock_noshared_mutex(&lockval));	// MUTEX LOCK
	}
	if(DTOR_INVALID_HANDLE == (opened_fd = get_exist_file(filepath.c_str(), opened_devid, opened_inode))){
		ERR_K2HPRN("Could not open file(%s).", filepath.c_str());
		fullock::flck_unlock_noshared_mutex(&lockval);			// MUTEX UNLOCK
		return false;
	}
	fullock::flck_unlock_noshared_mutex(&lockval);				// MUTEX UNLOCK

	return true;
}

bool K2htpSvrFd::RunThread(void)
{
	if(run_thread){
		//WAN_K2HPRN("Already run watch thread.");
		return true;
	}

	// run thread
	run_thread = true;
	int result;
	if(0 != (result = pthread_create(&watch_thread, NULL, K2htpSvrFd::WorkerThread, this))){
		ERR_K2HPRN("Could not create thread by errno(%d)", result);
		run_thread = false;
		return false;
	}
	MSG_K2HPRN("Succeed to run thread.");

	return true;
}

bool K2htpSvrFd::StopThread(void)
{
	if(!run_thread){
		//WAN_K2HPRN("Already stop watch thread.");
		return true;
	}
	run_thread = false;

	// wait for thread exit
	void*	pretval = NULL;
	int		result;
	if(0 != (result = pthread_join(watch_thread, &pretval))){
		ERR_K2HPRN("Failed to wait thread exiting by errno(%d), but continue,,,", result);
		run_thread = true;
		return false;
	}
	MSG_K2HPRN("Succeed to wait thread exiting.");
	return true;
}

bool K2htpSvrFd::Write(const unsigned char* pdata, size_t length)
{
	if(!pdata || 0 == length){
		ERR_K2HPRN("parameters are wrong.");
		return false;
	}
	if(!Open(false)){
		ERR_K2HPRN("could not open file(%s).", filepath.c_str());
		return false;
	}

	while(!fullock::flck_trylock_noshared_mutex(&lockval));	// MUTEX LOCK

	// seek to end
	if(-1 == lseek(opened_fd, 0, SEEK_END)){
		ERR_K2HPRN("Could not seek to file(%s) end by errno(%d).", filepath.c_str(), errno);
	}

	// write to file
	if(!k2htpdtor_write(opened_fd, pdata, length)){
		ERR_K2HPRN("Could not write to output file(%s) by errno(%d).", filepath.c_str(), errno);
		fullock::flck_unlock_noshared_mutex(&lockval);		// MUTEX UNLOCK
		return false;
	}
	fdatasync(opened_fd);
	fullock::flck_unlock_noshared_mutex(&lockval);			// MUTEX UNLOCK

	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
