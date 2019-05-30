/*
 *模块测试代码,redis 异步模块测试代码，主要测试redis异常、redis正常功能的代码
 *示例代码，也可以作为redis异步使用示例代码接入项目中去
 *
 *zhaoxiaoxiao 2019-05-24
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <string.h>
#include <pthread.h>
#include <iostream>

#include "RedisAsync.h"

#define PTRACE(fmt, args...)        fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PDEBUG(fmt, args...)        fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)        fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

static int singleFd = 0, clusterFd = 0;

static int sigArray[] = {
    SIGALRM,SIGQUIT,SIGILL,SIGTRAP,SIGABRT,SIGBUS,SIGFPE,
    SIGSEGV,SIGPIPE,SIGALRM,SIGCHLD,SIGCONT,SIGTSTP,
    SIGTTIN,SIGTTOU,SIGURG,SIGXCPU,SIGXFSZ,SIGVTALRM,SIGPROF,SIGWINCH,
    SIGIO,SIGPWR,SIGSYS
};

int daemonize_(int nochdir, int noclose)
{
    int fd;

    switch (fork())
    {
        case -1:
            {
                return (-1);
            }
        case 0:
            {
                break;
            }
        default:
            {
                _exit(EXIT_SUCCESS);
            }
    }

    if (setsid() == -1)
    {
        return (-1);
    }

    if (nochdir == 0)
    {
        if(chdir("/") != 0)
        {
            perror("chdir");
            return (-1);
        }
    }

    if (noclose == 0 && (fd = open("/dev/null", O_RDWR, 0)) != -1)
    {
        if(dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2 stdin");
            return (-1);
        }
        if(dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2 stdout");
            return (-1);
        }
        if(dup2(fd, STDERR_FILENO) < 0)
        {
            perror("dup2 stderr");
            return (-1);
        }

        if (fd > STDERR_FILENO)
        {
            if(close(fd) < 0)
            {
                perror("close");
                return (-1);
            }
        }
    }
    return (0);
}

inline int sigignore(int sig)
{
    struct sigaction sa;;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;

    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(sig, &sa, 0) == -1)
    {
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
        if(sigArray[i] == SIGCHLD)
        {
            signal(SIGCHLD, SIG_IGN);
            continue;
        }
        
        ret = sigignore(sigArray[i]);
        if(ret < 0)
        {
            return ret;
        }
    }

    return ret;
}

void sigMyself()
{
    pid_t self =  getpid();
    kill(self, SIGINT);
    return;
}

//捕获信号
void sig_catch(int sig)
{
    PERROR("!!!!!!!!well, we catch signal in this process :::%d will be exit",sig);
    switch(sig)
    {
        case SIGINT:
            PERROR("SIGINT :: %d", SIGINT);
            break;
        case SIGKILL:
            PERROR("SIGKILL :: %d", SIGKILL);
            break;
        case SIGTERM:
            PERROR("SIGTERM :: %d",SIGTERM);
            break;
        case SIGSTOP:
            PERROR("SIGSTOP :: %d",SIGSTOP);
            break;
        case SIGUSR1://10
            PERROR("SIGUSR1 :: %d",SIGUSR1);
            return;
        case SIGUSR2://12
            PERROR("SIGUSR2 :: %d",SIGUSR2);
            return;
        default:
            PERROR("%d",sig);
            break;
    }
    CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisAsyncExit();
    exit(0);
    return;
}

static void stateChangeCallback(int asyFd, int stateCode, const std::string& stateMsg)
{
    PDEBUG("asyFd %d stateCode %d stateMsg %s", asyFd, stateCode, stateMsg.c_str());
}

static void exceptionRedisMsg(int asyFd, int exceCode, const std::string& exceMsg)
{
    PDEBUG("asyFd %d exceCode %d exceMsg %s", asyFd, exceCode, exceMsg.c_str());
}

void cmdResultCallBack(int ret, void* priv, const CLUSTER_REDIS_ASYNC::StdVectorStringPtr& resultMsg)
{
    PDEBUG("ret %d priv %p", ret , priv);
    for(CLUSTER_REDIS_ASYNC::StdVectorStringPtr::const_iterator iter = resultMsg.begin(); iter != resultMsg.end(); iter++)
    {
        PDEBUG("\n%s", (*iter)->c_str());
    }
}

int main(int argc, char* argv[])
{
    int ret = IgnoreSig();
    if (ret == -1)
    {
        PERROR("Failed to ignore system signal\n");
        return ret;
    }

    (void)signal(SIGINT,sig_catch);//ctrl + c 
    //(void)signal(SIGKILL, sig_catch);//kill -9
    (void)signal(SIGTERM, sig_catch);//kill
    //(void)signal(SIGSTOP, sig_catch);
    (void)signal(SIGUSR1, sig_catch);
    (void)signal(SIGUSR2, sig_catch);
    
    CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisAsyncInit(2, 2, 3, 10,
        std::bind(stateChangeCallback, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3),
        std::bind(exceptionRedisMsg, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3));
    PDEBUG("RedisAsync init");

    singleFd = CLUSTER_REDIS_ASYNC::RedisAsync::instance().addSigleRedisInfo("127.0.0.1:6800");
    PDEBUG("RedisAsync addSigleRedisInfo singleFd %d", singleFd);
    if(singleFd < 0)
    {
        return -1;
    }

    clusterFd = CLUSTER_REDIS_ASYNC::RedisAsync::instance().addClusterInfo("192.169.6.234:6790,192.169.6.234:6791");
    PDEBUG("RedisAsync addSigleRedisInfo clusterFd %d", clusterFd);
    if(clusterFd < 0)
    {
        return -1;
    }

    while(1)
    {
        int redisFd = 0;
        std:: string inputLine, redisKey, redisCmd;
        std::cout << "please input operation redis fd:";
        
        std::getline(std::cin, inputLine);
        if(inputLine.empty())
        {
            std::cout << "input empty, please operation again" << std::endl;
            continue;
        }
        redisFd = atoi(inputLine.c_str());
        inputLine.assign("");

        std::cout << "please input operation redis operation key:";
        std::getline(std::cin, inputLine);
        if(inputLine.empty())
        {
            std::cout << "input empty, please operation again" << std::endl;
            continue;
        }
        redisKey.assign(inputLine);
        inputLine.assign("");

        std::cout << "please input operation redis operation cmd:";
        std::getline(std::cin, inputLine);
        if(inputLine.empty())
        {
            std::cout << "input empty, please operation again" << std::endl;
            continue;
        }
        redisCmd.assign(inputLine);
        inputLine.assign("");
        std::cout << "input redis fd " << redisFd << " redis operation key "<< redisKey << " and redis cmd "<< inputLine << "\".Are you sure?(y/n)" << std::endl;
        std::getline(std::cin, inputLine);
        if(inputLine.empty())
        {
            std::cout << "input empty, please operation again" << std::endl;
            continue;
        }else{
           if(inputLine.compare("y") == 0)
            {
                ret = CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisAsyncCommand(redisFd,
                    std::bind(cmdResultCallBack, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3),
                    3, nullptr, redisKey, redisCmd);
                PDEBUG("redisAsyncCommand %d", ret);
            }else{
                PDEBUG("You had just drop you cmd just now,please operation again");
            }
        }
    }
    return 0;
}

