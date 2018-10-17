#ifndef __XIAO_CURL_HTTP_CLI_H__
#define __XIAO_CURL_HTTP_CLI_H__

#include "src/Atomic.h"
#include "src/noncopyable.h"
#include "src/Singleton.h"
#include "src/AsyncCurlHttp.h"
#include "src/ThreadPool.h"
#include "HttpReqSession.h"

namespace CURL_HTTP_CLI
{

typedef AtomicIntegerT<uint32_t> AtomicUInt32;

/*
 *Http客户端，任何一个工程，全局单例。
 *每个请求对用一个回调，初始化请求session，(一定要设置回调)，回调会带回请求数据以及响应数据
 *包含此头文件，初始化好线程数，最大队列数，如果队列大小为0，则不受限制，内存会持续涨，设置值了，请求接口如果队列满了会阻塞
 *
 *使用方法:
 *先 curlHttpCliInit 之后 curlHttpRequest，接口是异步的，退出时一定要调用 curlHttpCliExit,
 *curlHttpCliExit是阻塞的，返回则说明所有请求都已经返回
 *由于每个请求的连接超时时间和数据超时时间都不一样，可能会curlHttpCliExit阻塞最大的超时时间
 *
 */
class CurlHttpCli : public noncopyable
{
public:

	CurlHttpCli();

	~CurlHttpCli();

	static CurlHttpCli& instance() { return Singleton<CurlHttpCli>::instance();}

	void curlHttpCliExit();

	int curlHttpCliInit(int threadNum, int maxQueue, int isShowTimeUse = 0);//最后一个参数在调试并发量的时候才会使用

	//程序阻塞，如果malloc失败返回-1
	//如果程序已经收到退出命令返回-2
	int curlHttpRequest(HttpReqSession& curlReq);//阻塞方法，如果队列满了，会一直阻塞

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//内部使用，外部使用不需要使用
	void curlHttpClientWakeup();

	void curlHttpThreadReady();

private:

	void httpIoThreadFun(int index);

	void httpStatisticsSecond();

	void curlHttpThreadExit();

	int threadNum_;
	int isExit_;
	int isReady;
	int isShowtime;
	AtomicUInt32 readyNum;
	AtomicUInt32 exitNum;
	AtomicUInt32 lastIndex;
	std::unique_ptr<ThreadPool> httpCliPoolPtr;
	std::vector<AsyncCurlHttp*> curlCliVect;
};

}
#endif