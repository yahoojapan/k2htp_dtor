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
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <k2hash/k2hutil.h>
#include <k2hash/k2hdbg.h>
#include <chmpx/chmpx.h>
#include <chmpx/chmutil.h>
#include <fullock/flckstructure.h>
#include <fullock/flckbaselist.tcc>
#include <fullock/flckutil.h>

#include "k2htpdtorsvrplugin.h"
#include "k2htpdtorfile.h"

using namespace std;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	CHILD_READ_FD_PIPE_POS			0
#define	PARENT_WRITE_FD_PIPE_POS		(CHILD_READ_FD_PIPE_POS + 1)
#define	PIPE_FD_COUNT					(PARENT_WRITE_FD_PIPE_POS + 1)

#define	K2HFUSE_NPIPE_BASE_DIR			"/var/lib/antpickax"
#define	K2HFUSE_NPIPE_TMP_DIR			"/tmp"
#define	K2HFUSE_NPIPE_FILENAME_PREFIX	"k2htpdtorsvr_np_"

#define	MAX_WAIT_COUNT_UNTIL_KILL		500			// 500 * 1ms = 500ms

//---------------------------------------------------------
// Utilities
//---------------------------------------------------------
#define	SPACE_CAHRS_FOR_PARSER		" \t\r\n"

//
// If list is empty list, returns array pointer which has one char*(NULL).
//
static char** convert_strlst_to_chararray(const strlst_t& list)
{
	char**	ppstrings;
	if(NULL == (ppstrings = static_cast<char**>(calloc((list.size() + 1), sizeof(char*))))){	// null terminate
		ERR_K2HPRN("could not allocate memory.");
		return NULL;
	}
	char**	pptmp = ppstrings;
	for(strlst_t::const_iterator iter = list.begin(); iter != list.end(); ++iter){
		if((*iter).empty()){
			continue;	// why...
		}
		if(NULL == (*pptmp = strdup((*iter).c_str()))){
			ERR_K2HPRN("could not allocate memory.");
			continue;	// uh...
		}
		pptmp++;
	}
	return ppstrings;
}

static int convert_string_to_strlst(const char* base, strlst_t& list)
{
	list.clear();
	if(ISEMPTYSTR(base)){
		MSG_K2HPRN("base parameter is empty.");
		return 0;
	}
	char*	tmpbase = strdup(base);
	char*	tmpsave = NULL;
	for(char* tmptok = strtok_r(tmpbase, SPACE_CAHRS_FOR_PARSER, &tmpsave); tmptok; tmptok = strtok_r(NULL, SPACE_CAHRS_FOR_PARSER, &tmpsave)){
		if(ISEMPTYSTR(tmptok)){
			// empty string
			continue;
		}
		list.push_back(string(tmptok));
	}
	K2H_Free(tmpbase);

	return list.size();
}

static void free_chararray(char** pparray)
{
	for(char** pptmp = pparray; pptmp && *pptmp; ++pptmp){
		K2H_Free(*pptmp);
	}
	K2H_Free(pparray);
}

//---------------------------------------------------------
// Class methods
//---------------------------------------------------------
bool K2htpSvrPlugin::BlockSignal(int sig)
{
	sigset_t	blockmask;
	sigemptyset(&blockmask);
	sigaddset(&blockmask, sig);
	if(-1 == sigprocmask(SIG_BLOCK, &blockmask, NULL)){
		ERR_K2HPRN("Could not block signal(%d) by errno(%d)", sig, errno);
		return false;
	}
	return true;
}

bool K2htpSvrPlugin::ClearFdFlags(int fd, int flags)
{
	int	value;
	if(-1 == (value = fcntl(fd, F_GETFL, 0))){
		ERR_K2HPRN("Could not get file open flags by errno(%d)", errno);
		return false;
	}
	value &= ~flags;
	if(-1 == (value = fcntl(fd, F_SETFL, value))){
		ERR_K2HPRN("Could not clear file open flags(clear %d, result %d) by errno(%d)", flags, value, errno);
		return false;
	}
	return true;
}

void* K2htpSvrPlugin::WatchChildExit(void* param)
{
	if(!param){
		ERR_K2HPRN("param is NULL.");
		pthread_exit(NULL);
	}
	K2htpSvrPlugin*	pPlugin = static_cast<K2htpSvrPlugin*>(param);

	// block SIGCHLD
	if(!K2htpSvrPlugin::BlockSignal(SIGCHLD)){
		ERR_K2HPRN("Could not block SIGCHLD.");
		pthread_exit(NULL);
	}

	// loop
	// cppcheck-suppress knownConditionTrueFalse
	while(!pPlugin->thread_exit){
		// blocking child exit
		int		status	= 0;
		pid_t	childpid= waitpid(-1, &status, 0);

		// check before run plugins
		// cppcheck-suppress knownConditionTrueFalse
		if(pPlugin->thread_exit){
			break;
		}
		if(0 < childpid){
			// child exit
			if(pPlugin->PluginPid == childpid){
				ERR_K2HPRN("Caught plugin(%s) exited with status(%d).", pPlugin->BaseParam.c_str(), WEXITSTATUS(status));

				while(!fullock::flck_trylock_noshared_mutex(&(pPlugin->lockval)));	// MUTEX LOCK

				pPlugin->PluginPid	= CHM_INVALID_PID;
				pPlugin->ExitStatus	= WEXITSTATUS(status);
				pPlugin->ExitCount++;
				// [NOTE]
				// do not close pipe for writer

				fullock::flck_unlock_noshared_mutex(&(pPlugin->lockval));			// MUTEX UNLOCK
			}else{
				WAN_K2HPRN("child(pid=%d) exited, but unknown pid(my child pid=%d).", childpid , pPlugin->PluginPid);
			}

		}else if(-1 > childpid){
			if(ECHILD != errno){
				// error
				MSG_K2HPRN("something error occurred during waiting child exit by errno(%d).", errno);
				break;
			}
			// [NOTE]
			// we set SIG_IGN to SIGCHILD, thus we caught this error after all children exited.
			// this case is no problem.

		}else if(0 == childpid){
			// no child exit
			//MSG_K2HPRN("no child is status changed.");
		}

		// check before run plugins
		if(pPlugin->thread_exit){
			break;
		}

		// run plugin(run as soon as possible)
		if(0 < childpid && !pPlugin->RunPlugin()){
			ERR_K2HPRN("Something error occurred during restarting plugins, but continue...");
		}
	}
	pthread_exit(NULL);
	return NULL;
}

//---------------------------------------------------------
// Methods
//---------------------------------------------------------
K2htpSvrPlugin::K2htpSvrPlugin(void) : lockval(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED), thread_exit(true), run_thread_status(false), BaseParam(""), FileName(""), PipeFilePath(""), PluginArgvs(NULL), PluginEnvs(NULL), PluginPid(CHM_INVALID_PID), PipeInput(DTOR_INVALID_HANDLE), ExitStatus(0), ExitCount(0), AutoRestart(false)
{
	// if named pipe file exists, remove it.
	RemoveNamedPipeFile();
}

K2htpSvrPlugin::~K2htpSvrPlugin(void)
{
	if(!Clean()){
		ERR_K2HPRN("Failed to cleanup this class.");
	}
}

bool K2htpSvrPlugin::Clean(void)
{
	bool	result = true;

	// stop plugin
	if(!StopPlugin()){
		ERR_K2HPRN("Failed to stop plugin, but continue...");
		result = false;
	}
	free_chararray(PluginArgvs);
	free_chararray(PluginEnvs);
	PluginArgvs	= NULL;
	PluginEnvs	= NULL;

	// stop watch thread
	if(!StopWatchThread()){
		ERR_K2HPRN("Failed to stop watch thread, but continue...");
		result = false;
	}

	// if named pipe is opened, close it.
	K2H_CLOSE(PipeInput);

	// remove named pipe file
	RemoveNamedPipeFile();

	return result;
}

bool K2htpSvrPlugin::Initialize(const char* base)
{
	if(ISEMPTYSTR(base)){
		ERR_K2HPRN("parameter is wrong.");
		return false;
	}
	if(CHM_INVALID_PID != PluginPid){
		ERR_K2HPRN("plugin process is running now.");
		return false;
	}
	K2H_CLOSE(PipeInput);

	// parse plugin command line
	if(!ParseExecParam(base)){
		ERR_K2HPRN("plugin base string(%s) is wrong.", base);
		return false;
	}

	// block SIGPIPE
	if(!K2htpSvrPlugin::BlockSignal(SIGPIPE)){
		ERR_K2HPRN("Could not block SIGPIPE.");
		return false;
	}

	// block SIGCHLD
	if(!K2htpSvrPlugin::BlockSignal(SIGCHLD)){
		ERR_K2HPRN("Could not block SIGCHLD.");
		return false;
	}

	// initialize internal data
	BaseParam	= base;
	PluginPid	= CHM_INVALID_PID;
	PipeInput	= DTOR_INVALID_HANDLE;
	ExitStatus	= 0;
	ExitCount	= 0;
	AutoRestart	= true;

	// build named pipe(if it exists, do not remove it.)
	if(!BuildNamedPipeFile(false)){
		ERR_K2HPRN("Could not build named pipe file.");
		return false;
	}

	// run plugin
	if(!RunPlugin()){
		ERR_K2HPRN("could not run plugin(%s).", FileName.c_str());
		Clean();
		return false;
	}

	// run watch thread
	if(!RunWatchThread()){
		ERR_K2HPRN("Could not create watch thread.");
		return false;
	}
	return true;
}

bool K2htpSvrPlugin::ParseExecParam(const char* base)
{
	if(ISEMPTYSTR(base)){
		ERR_K2HPRN("base parameter is empty.");
		return false;
	}
	// convert
	strlst_t	baselist;
	if(0 == convert_string_to_strlst(base, baselist)){
		ERR_K2HPRN("There is no string token in base parameter.");
		return false;
	}

	// parse string list to filename/argvs/envs
	strlst_t	argvs;
	strlst_t	envs;
	for(strlst_t::const_iterator iter = baselist.begin(); iter != baselist.end(); ++iter){
		if(string::npos != (*iter).find('=')){
			// env parameter
			envs.push_back(*iter);
		}else{
			// filename
			FileName = (*iter);

			// all string list after filename are argv.
			argvs.push_back(FileName);		// 1'st argv is filename
			for(++iter; iter != baselist.end(); ++iter){
				argvs.push_back(*iter);
			}
			break;
		}
	}

	// make char array
	free_chararray(PluginArgvs);
	free_chararray(PluginEnvs);
	PluginArgvs	= convert_strlst_to_chararray(argvs);
	PluginEnvs	= convert_strlst_to_chararray(envs);

	return (!FileName.empty());
}

bool K2htpSvrPlugin::RunWatchThread(void)
{
	if(run_thread_status){
		//WAN_K2HPRN("Already run watch thread.");
		return true;
	}

	// run thread
	thread_exit = false;
	int result;
	if(0 != (result = pthread_create(&watch_thread_tid, NULL, K2htpSvrPlugin::WatchChildExit, this))){
		ERR_K2HPRN("could not create watch children thread by errno(%d)", result);
		return false;
	}
	run_thread_status = true;

	return true;
}

bool K2htpSvrPlugin::StopWatchThread(void)
{
	if(!run_thread_status){
		//WAN_K2HPRN("Already stop watch thread.");
		return true;
	}

	// set stop thread flag
	thread_exit = true;

	// wait for exiting thread
	int		result;
	void*	pretval = NULL;
	if(0 != (result = pthread_join(watch_thread_tid, &pretval))){
		ERR_K2HPRN("Failed to wait thread exit by errno(%d), but continue...", result);
		return false;
	}
	run_thread_status = false;

	return true;
}

bool K2htpSvrPlugin::BuildNamedPipeFile(bool keep_exist)
{
	static bool		stc_init_basedir = false;
	static string	stc_basedir;

	if(!stc_init_basedir){
		struct stat	st;
		if(0 != stat(K2HFUSE_NPIPE_BASE_DIR, &st)){
			WAN_K2HPRN("%s directory is not existed, then use %s", K2HFUSE_NPIPE_BASE_DIR, K2HFUSE_NPIPE_TMP_DIR);
			stc_basedir = K2HFUSE_NPIPE_TMP_DIR;
		}else{
			if(0 == (st.st_mode & S_IFDIR)){
				WAN_K2HPRN("%s is not directory, then use %s", K2HFUSE_NPIPE_BASE_DIR, K2HFUSE_NPIPE_TMP_DIR);
				stc_basedir = K2HFUSE_NPIPE_TMP_DIR;
			}else{
				stc_basedir = K2HFUSE_NPIPE_BASE_DIR;
			}
		}
		stc_init_basedir = true;
	}

	if(PipeFilePath.empty()){
		PipeFilePath  = stc_basedir;
		PipeFilePath += "/";
		PipeFilePath += K2HFUSE_NPIPE_FILENAME_PREFIX;
		PipeFilePath += to_string(getpid());
	}

	// check file
	struct stat	st;
	if(0 == stat(PipeFilePath.c_str(), &st)){
		if(keep_exist && S_ISFIFO(st.st_mode)){
			MSG_K2HPRN("Named pipe file(%s) already exists, so keep it.", PipeFilePath.c_str());
			return true;
		}
		if(0 != unlink(PipeFilePath.c_str())){
			ERR_K2HPRN("Named pipe file(%s) exists, but remove(initiate) it. errno(%d)", PipeFilePath.c_str(), errno);
			return false;
		}
	}

	// make named pipe
	if(-1 == mkfifo(PipeFilePath.c_str(), S_IRUSR | S_IWUSR)){
		ERR_K2HPRN("could not make named pipe(%s) by errno(%d).", PipeFilePath.c_str(), errno);
		return false;
	}
	return true;
}

bool K2htpSvrPlugin::RemoveNamedPipeFile(void)
{
	if(PipeFilePath.empty()){
		MSG_K2HPRN("Named pipe file path is empty.");
		return true;
	}

	struct stat	st;
	if(0 != stat(PipeFilePath.c_str(), &st)){
		MSG_K2HPRN("Named pipe file(%s) does not exist.", PipeFilePath.c_str());
		return true;
	}
	if(0 != unlink(PipeFilePath.c_str())){
		ERR_K2HPRN("Named pipe file(%s) exists, but remove(initiate) it. errno(%d)", PipeFilePath.c_str(), errno);
		return false;
	}
	return true;
}

bool K2htpSvrPlugin::RunPlugin(void)
{
	if(BaseParam.empty()){
		ERR_K2HPRN("There is no plugin.");
		return false;
	}

	while(!fullock::flck_trylock_noshared_mutex(&lockval));	// MUTEX LOCK

	// check auto restart flag
	if(!AutoRestart){
		ERR_K2HPRN("plugin auto restart flag is false, so stop to run plugin.");
		fullock::flck_unlock_noshared_mutex(&lockval);		// MUTEX UNLOCK
		return true;
	}

	// build named pipe(if it exists, do not remove it.)
	if(!BuildNamedPipeFile(true)){
		ERR_K2HPRN("Could not build named pipe file.");
		fullock::flck_unlock_noshared_mutex(&lockval);		// MUTEX UNLOCK
		return false;
	}

	// fork
	pid_t	childpid = fork();
	if(-1 == childpid){
		// error
		ERR_K2HPRN("Could not fork for \"%s\" plugin by errno(%d).", BaseParam.c_str(), errno);
		ExitCount++;
		ExitStatus	= -errno;
		fullock::flck_unlock_noshared_mutex(&lockval);		// MUTEX UNLOCK
		return false;

	}else if(0 == childpid){
		// child process

		// for open named PIPE without blocking
		int	child_input_pipe;
		{
			// open READ mode with nonblocking
			if(-1 == (child_input_pipe = open(PipeFilePath.c_str(), O_RDONLY | O_NONBLOCK))){
				ERR_K2HPRN("Could not open read(non blocking) only named pipe(%s) by errno(%d).", PipeFilePath.c_str(), errno);
				exit(EXIT_FAILURE);
			}
			// open WRITE with blocking(never use and close this)
			int	child_wr_pipe;
			if(-1 == (child_wr_pipe = open(PipeFilePath.c_str(), O_WRONLY))){
				ERR_K2HPRN("Could not open write only named pipe(%s) by errno(%d).", PipeFilePath.c_str(), errno);
				K2H_CLOSE(child_input_pipe);
				exit(EXIT_FAILURE);
			}
			// set blocking mode to READ pipe
			if(!K2htpSvrPlugin::ClearFdFlags(child_input_pipe, O_NONBLOCK)){
				ERR_K2HPRN("Could not unset blocking to readable named pipe(%s).", PipeFilePath.c_str());
				K2H_CLOSE(child_input_pipe);
				K2H_CLOSE(child_wr_pipe);
				exit(EXIT_FAILURE);
			}
			// close writer fd
			K2H_CLOSE(child_wr_pipe);
		}

		// signal
		{
			// unblock
			sigset_t	blockmask;
			sigemptyset(&blockmask);
			sigaddset(&blockmask, SIGPIPE);
			sigaddset(&blockmask, SIGCHLD);
			if(-1 == sigprocmask(SIG_UNBLOCK, &blockmask, NULL)){
				ERR_K2HPRN("Could not block SIGPIPE/CHLD by errno(%d)", errno);
			}

			// set default handlers
			struct sigaction	saDefault;
			sigemptyset(&saDefault.sa_mask);
			saDefault.sa_handler= SIG_DFL;
			saDefault.sa_flags	= 0;
			if(	-1 == sigaction(SIGINT, &saDefault, NULL)	||
				-1 == sigaction(SIGQUIT, &saDefault, NULL)	||
				-1 == sigaction(SIGCHLD, &saDefault, NULL)	||
				-1 == sigaction(SIGPIPE, &saDefault, NULL)	)
			{
				ERR_K2HPRN("Could not set default signal handler(INT/QUIT/CHLD/PIPE) for \"%s\" plugin by errno(%d), but continue...", FileName.c_str(), errno);
			}
		}

		// duplicate pipe/fd to stdin/stdout
		if(-1 == dup2(child_input_pipe, STDIN_FILENO)){
			ERR_K2HPRN("could not duplicate stdin for \"%s\" plugin by errno(%d).", BaseParam.c_str(), errno);
			K2H_CLOSE(child_input_pipe);
			ExitCount++;
			ExitStatus	= -errno;
			exit(EXIT_FAILURE);
		}
		// close all
		K2H_CLOSE(child_input_pipe);

		MSG_K2HPRN("CHILD PROCESS: Started child plugin(%s) process(pid=%d).", FileName.c_str(), getpid());

		// execute
		if(-1 == execve(FileName.c_str(), PluginArgvs, PluginEnvs)){
			ERR_K2HPRN("could not execute \"%s\" plugin by errno(%d).", BaseParam.c_str(), errno);
			ExitCount++;
			ExitStatus	= -errno;
			exit(EXIT_FAILURE);
		}

	}else{
		// parent process
		PluginPid = childpid;

		// open input pipe, if it is not opened.
		if(DTOR_INVALID_HANDLE == PipeInput){
			if(DTOR_INVALID_HANDLE == (PipeInput = open(PipeFilePath.c_str(), O_WRONLY))){
				ERR_K2HPRN("Could not open write only named pipe(%s) by errno(%d).", PipeFilePath.c_str(), errno);
				ExitCount++;
				ExitStatus	= -errno;
				fullock::flck_unlock_noshared_mutex(&lockval);	// MUTEX UNLOCK
				return false;
			}
		}
	}
	fullock::flck_unlock_noshared_mutex(&lockval);				// MUTEX UNLOCK

	return true;
}

bool K2htpSvrPlugin::StopPlugin(void)
{
	if(CHM_INVALID_PID == PluginPid){
		WAN_K2HPRN("pid is not running status, so already stop.");
		return true;
	}

	// check process is running
	if(-1 == kill(PluginPid, 0)){
		ERR_K2HPRN("process(%d) does not allow signal or not exist, by errno(%d)", PluginPid, errno);
		return false;
	}

	// no restart
	while(!fullock::flck_trylock_noshared_mutex(&lockval));	// MUTEX LOCK
	AutoRestart = false;
	fullock::flck_unlock_noshared_mutex(&lockval);			// MUTEX UNLOCK

	// do stop(SIGHUP)
	if(-1 == kill(PluginPid, SIGHUP)){
		ERR_K2HPRN("process(%d) does not allow signal or not exist, by errno(%d)", PluginPid, errno);
		return false;
	}

	// wait for exiting(waitpid is called in watch thread)
	for(int cnt = 0; CHM_INVALID_PID != PluginPid; ++cnt){
		if(MAX_WAIT_COUNT_UNTIL_KILL == cnt){
			// do stop(SIGKILL)
			MSG_K2HPRN("could not stop process(%d) by SIGHUP, so try to send SIGKILL.", PluginPid);

			if(-1 == kill(PluginPid, SIGKILL)){
				ERR_K2HPRN("process(%d) does not allow signal or not exist, by errno(%d)", PluginPid, errno);
			}

		}else if((MAX_WAIT_COUNT_UNTIL_KILL * 2) < cnt){
			ERR_K2HPRN("gave up to stop process(%d)", PluginPid);

			// force set
			while(!fullock::flck_trylock_noshared_mutex(&lockval));	// MUTEX LOCK

			PluginPid	= CHM_INVALID_PID;
			ExitStatus	= -1;										// means error
			ExitCount++;
			// [NOTE]
			// do not close pipe for writer

			fullock::flck_unlock_noshared_mutex(&lockval);			// MUTEX UNLOCK

			return false;
		}
		// sleep
		struct timespec	sleeptime = {0L, 1 * 1000 * 1000};			// wait 1ms
		nanosleep(&sleeptime, NULL);
	}

	MSG_K2HPRN("stopped plugin(%d).", PluginPid);

	return true;
}

bool K2htpSvrPlugin::Write(const unsigned char* pdata, size_t length)
{
	if(!pdata || 0 == length){
		ERR_K2HPRN("parameters are wrong.");
		return false;
	}
	if(DTOR_INVALID_HANDLE == PipeInput){
		ERR_K2HPRN("The input pipe for plugin is invalid, could not write data to plugin.");
		return false;
	}

	// do write
	if(!k2htpdtor_write(PipeInput, pdata, length)){
		ERR_K2HPRN("Could not write to plugin input pipe(%d) by errno(%d).", PipeInput, errno);
		return false;
	}
	return true;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
