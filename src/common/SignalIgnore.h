#ifndef __RCS_SIGNAL_IGNORE_H__
#define __RCS_SIGNAL_IGNORE_H__

#include <signal.h>

namespace common
{
namespace sigignore
{

//SIGINT,SIGKILL,SIGTERM,SIGSTOP,SIGUSR1,SIGUSR2
static int sigArray[] = {
	SIGALRM,SIGQUIT,SIGILL,SIGTRAP,SIGABRT,SIGBUS,SIGFPE,
	SIGSEGV,SIGPIPE,SIGALRM,SIGCHLD,SIGCONT,SIGTSTP,
	SIGTTIN,SIGTTOU,SIGURG,SIGXCPU,SIGXFSZ,SIGVTALRM,SIGPROF,SIGWINCH,
	SIGIO,SIGPWR,SIGSYS
};

inline int sigignore(int sig)
{
    struct sigaction sa;;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;

    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(sig, &sa, 0) == -1) {
        return -1;
    }
    return 0;
}

inline int IgnoreSig()
{
	int ret = 0;
	size_t size = sizeof(sigArray)/sizeof(int);
	for(size_t i = 0;i < size;i++)
	{
		ret = sigignore(sigArray[i]);
		if(ret < 0)
		{
			return ret;
		}
	}

	return ret;
}

}
}

#endif