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

#ifndef	K2HTPDTORAVRPLUGIN_H
#define K2HTPDTORAVRPLUGIN_H

#include <string>

//---------------------------------------------------------
// Class K2htpSvrPlugin
//---------------------------------------------------------
class K2htpSvrPlugin
{
	protected:
		volatile int	lockval;			// the value to run_plugin and stop_plugin
		volatile bool	thread_exit;		// request flag for watching children thread exit
		volatile bool	run_thread_status;	// thread status(run/stop)
		pthread_t		watch_thread_tid;	// watch children thread id

		// plugin data
		std::string		BaseParam;
		std::string		FileName;
		std::string		PipeFilePath;		// named pipe file path for plugin input
		char**			PluginArgvs;
		char**			PluginEnvs;
		pid_t			PluginPid;
		int				PipeInput;
		int				ExitStatus;
		int				ExitCount;
		bool			AutoRestart;

	protected:
		static bool BlockSignal(int sig);
		static bool ClearFdFlags(int fd, int flags);
		static void* WatchChildExit(void* param);

		bool ParseExecParam(const char* base);
		bool RunWatchThread(void);
		bool StopWatchThread(void);
		bool BuildNamedPipeFile(bool keep_exist);
		bool RemoveNamedPipeFile(void);
		bool RunPlugin(void);
		bool StopPlugin(void);

	public:
		K2htpSvrPlugin(void);
		virtual ~K2htpSvrPlugin(void);

		bool Clean(void);
		bool Initialize(const char* base);

		bool Write(const unsigned char* pdata, size_t length);
};

#endif	// K2HTPDTORAVRPLUGIN_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
