/*
 *模块测试代码，测试curl http cli端模块代码、性能、功能
 *示例代码，如何正确的将模块代码集成到项目中，如何保证数据不丢、不重复、程序正常退出
 *
 *zhaoxiaoxiao
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

#include "HttpReqSession.h"
#include "CurlHttpCli.h"


#define CURL_HTTP_CLIENT_TEST_VERSION			"v1.0.0.0"

#define PDEBUG(fmt, args...)		fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)		fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

static int isExit = 0,isStopTest = 0;

//SIGINT,SIGKILL,SIGTERM,SIGSTOP,SIGUSR1,SIGUSR2
static int sigArray[] = {
	SIGALRM,SIGQUIT,SIGILL,SIGTRAP,SIGABRT,SIGBUS,SIGFPE,
	SIGSEGV,SIGPIPE,SIGALRM,SIGCHLD,SIGCONT,SIGTSTP,
	SIGTTIN,SIGTTOU,SIGURG,SIGXCPU,SIGXFSZ,SIGVTALRM,SIGPROF,SIGWINCH,
	SIGIO,SIGPWR,SIGSYS
};

int daemonize_(int nochdir, int noclose)
{
    int fd;

    switch (fork()) {
    case -1:
        return (-1);
    case 0:
        break;
    default:
        _exit(EXIT_SUCCESS);
    }

    if (setsid() == -1)
        return (-1);

    if (nochdir == 0) {
        if(chdir("/") != 0) {
            perror("chdir");
            return (-1);
        }
    }

    if (noclose == 0 && (fd = open("/dev/null", O_RDWR, 0)) != -1) {
        if(dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2 stdin");
            return (-1);
        }
        if(dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 stdout");
            return (-1);
        }
        if(dup2(fd, STDERR_FILENO) < 0) {
            perror("dup2 stderr");
            return (-1);
        }

        if (fd > STDERR_FILENO) {
            if(close(fd) < 0) {
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

////程序运行中，内部调用终止程序运行的，慎用
void sigMyself()
{
	pid_t self =  getpid();
	kill(self,SIGINT);
	return;
}

//程序退出的时候退出之前清楚异步数据,处理模块内部异步数据
void processExit()
{
	isExit = 1;
	CURL_HTTP_CLI::CurlHttpCli::instance().curlHttpCliExit();//清除异步数据
}
//捕获信号
void sig_catch(int sig)
{
	PERROR("!!!!!!!!well, we catch signal in this process :::%d will be exit",sig);
	switch(sig)
	{
		case SIGINT:
			PERROR("SIGINT :: %d",SIGINT);
			if(isStopTest == 0)
			{
				isStopTest = 1;
				return;
			}
			break;
		case SIGKILL:
			PERROR("SIGKILL :: %d",SIGKILL);
			if(isStopTest == 0)
			{
				isStopTest = 1;
				return;
			}
			break;
		case SIGTERM:
			PERROR("SIGTERM :: %d",SIGTERM);
			if(isStopTest == 0)
			{
				isStopTest = 1;
				return;
			}
			break;
		case SIGSTOP:
			PERROR("SIGSTOP :: %d",SIGSTOP);
			if(isStopTest == 0)
			{
				isStopTest = 1;
				return;
			}
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
	#if 0
	pid_t self =  getpid();
	kill(self,SIGKILL);
	#else
	processExit();
	exit(0);
	#endif
	return;
}

static void httpReqCallback(CURL_HTTP_CLI::HttpReqSession* rsp)//HTTP响应回调,不要关系释放内存问题
{
	if(rsp->httpResponstCode() == 200)
	{
		PDEBUG("Http request respond callback code %d %s", rsp->httpResponstCode(), rsp->httpResponseData().c_str());
	}else{
		PDEBUG("Http request respond callback code %d %s", rsp->httpResponstCode(), rsp->httpReqErrorMsg().c_str());
	}
	return;
}

void* httpReqTest(void* arg)///HTTP请求
{
	int ret = 0;
	CURL_HTTP_CLI::HttpReqSession httpreq(HTTP11, HTTP_GET, "http://192.169.0.61:61888");
	httpreq.addHttpReqPrivateHead("Connection: close");
	httpreq.setHttpReqCallback(std::bind(&httpReqCallback, std::placeholders::_1));//注册回调
	while(1)
	{
		if(isExit || isStopTest)
		{
			break;
		}

		for(int i = 0; i < 50; i++)
		{
			ret = CURL_HTTP_CLI::CurlHttpCli::instance().curlHttpRequest(httpreq);
			if(ret < 0)
			{
				PERROR("CURL_HTTP_CLI::CurlHttpCli::instance().curlHttpRequest %d", ret);
			}
			if(isExit || isStopTest)
			{
				break;
			}
		}
		if(isExit || isStopTest)
		{
			break;
		}
		sleep(1);
	}
	return NULL;
}

int main(int argc, char* argv[])
{
	int ret = 0;
	pthread_t pthreadId_;
	
	fprintf(stderr, "curlHttpCliTest module version :: %s start\n", CURL_HTTP_CLIENT_TEST_VERSION);
	if (argc == 2)
	{
		if (memcmp(argv[1], "-v", 2) == 0 || memcmp(argv[1], "--version", 9) == 0)
		{
			fprintf(stderr, "curl test module version :: %s start\n", CURL_HTTP_CLIENT_TEST_VERSION);
		}
		else
		{
			fprintf(stderr, "Usage: ./curlHttpCliTest -v\n");
			fprintf(stderr, "       ./curlHttpCliTest --version\n");
		}
		return ret;
	}else if(argc == 1)
	{
		daemonize_(1,0);
	}

	ret = IgnoreSig();
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
	
	ret = CURL_HTTP_CLI::CurlHttpCli::instance().curlHttpCliInit(4, 50000, 10 , 0 ,0 , 1);//模块初始化
	if(ret < 0)
	{
		PERROR("CURL_HTTP_CLI::CurlHttpCli::instance().curlHttpCliInit %d", ret);
		return ret;
	}
	
	if(pthread_create(&pthreadId_, NULL, &httpReqTest, NULL))
	{
		PERROR("httpReqTest init error \n");
		exit(1);
	}
	while(1)
	{
		sleep(60);
	}
	return ret;
}
