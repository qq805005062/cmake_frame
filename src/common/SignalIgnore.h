#ifndef __COMMON_SIGNAL_IGNORE_H__
#define __COMMON_SIGNAL_IGNORE_H__

#include <signal.h>

namespace common
{
namespace sigignore
{

//SIGHUP SIGINT SIGQUIT SIGKILL SIGUSR1 SIGUSR2 SIGTERM SIGSTOP SIGWINCH SIGPWR
static int sigArray[] = {
    SIGILL,SIGTRAP,SIGABRT,SIGBUS,SIGFPE,
    SIGSEGV,SIGPIPE,SIGALRM,SIGSTKFLT,SIGCHLD,SIGCONT,SIGTSTP,
    SIGTTIN,SIGTTOU,SIGURG,SIGXCPU,SIGXFSZ,SIGVTALRM,SIGPROF,
    SIGIO,SIGSYS,SIGRTMIN
};

/*
 * [sigignore] 忽略对应的信号量
 * @author xiaoxiao 2019-04-02
 * @param sig 需要忽略的信号量常量值
 *
 * @return 执行是否成功，0是成功，其他失败
 */
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

/*
 * [IgnoreSig] 忽略相关进程信号方法，内部会将需要忽略的信号遍历忽略
 * @author xiaoxiao 2019-04-02
 * @param 无
 *
 * @return执行是否成功，0是成功，其他失败
 */
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

}// end namespace common

#endif //end __COMMON_SIGNAL_IGNORE_H__

