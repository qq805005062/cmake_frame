#ifndef __XIAO_CURL_HTTP_CLI_H__
#define __XIAO_CURL_HTTP_CLI_H__

#include <mutex>

#include "src/noncopyable.h"
#include "src/Singleton.h"

#include "HttpReqSession.h"


#define ABSOLUTELY_OUT_HTTP_SECOND		180
#define MAX_SIMULTANEOUSLY_CONNS		10000

namespace CURL_HTTP_CLI
{

class CurlHttpCli : public noncopyable
{
public:

	CurlHttpCli();

	~CurlHttpCli();

	static CurlHttpCli& instance() { return Singleton<CurlHttpCli>::instance();}

	//程序退出时调用方法，会清楚所有异步数据之后退出，阻塞方法，返回则表示数据已经清除了
	void curlHttpCliExit();

	int curlHttpCliInit(unsigned int threadNum, unsigned int maxQueue, unsigned int isKeepAlive = 0, unsigned int maxConns = MAX_SIMULTANEOUSLY_CONNS,unsigned int maxSpeed = 0);

	//程序阻塞，如果malloc失败返回-1
	//如果程序已经收到退出命令返回-2
	//此接口为阻塞的。如果队列大小满了，就会一直阻塞方法中不返回，可以修改为满了就立马退出来
	int curlHttpRequest(HttpReqSession& curlReq);

	////下面方法内部使用，上层应用不可使用，无需关心
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//内部使用，外部使用不需要使用
	void curlHttpClientWakeup();

	void curlHttpThreadReady();

	size_t httpIoThreadNum()
	{
		size_t num = static_cast<size_t>(ioThreadNum);
		return num;
	}

	unsigned int httpIsKeepAlive()
	{
		return isKeepAlive;
	}
private:

	void httpCliIoThread(size_t index);

	void httpRspCallBackThread();

	void httpOutRspCallBackThread();

	int isExit;
	int threadExit;
	unsigned int readyIothread;
	unsigned int exitIothread;
	unsigned int isKeepAlive;
	unsigned int lastIndex;
	unsigned int ioThreadNum;
	
	std::mutex mutex_;
};

}

#endif
