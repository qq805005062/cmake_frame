#ifndef __XIAO_CURL_HTTP_CLI_H__
#define __XIAO_CURL_HTTP_CLI_H__

#include "src/Atomic.h"
#include "src/noncopyable.h"
#include "src/Singleton.h"
#include "src/AsyncCurlHttp.h"
#include "src/Thread.h"
#include "src/ThreadPool.h"
#include "HttpReqSession.h"

#define ABSOLUTELY_OUT_HTTP_SECOND				180

namespace CURL_HTTP_CLI
{

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

	//程序退出时调用方法，会清楚所有异步数据之后退出，阻塞方法，返回则表示数据已经清除了
	void curlHttpCliExit();

	//第一个参数为IO线程数，内部会启动1个线程，回调在另外一个线程处理,同时处理超时无响应的连接，建议 2
	//第二个参数是最大的队列大小，内部缓冲队列大小，请求发起时会放入一个队列，如果为 0，不限制大小。如果比较大，而且入口大于出口速度，响应延迟可能就会很大
	//第三个参数限制每次最大的连接数，在同一个时刻，不会有超过这个数字的连接存在。为 0 则不限制，如果有一批连接堵住了，影响蛮大的
	//第四换参数限制每次每个线程最多建立多少个连接，这个限制作用影响比较上面参数弱。但是没有办法限制同一个时刻最大的连接数，如果有连接堵住了，不会影响后续新请求发出去
	//		如果出口比较满的慢的话，并且连接池中等待太多，出来一个连接也会立马加入一个连接，限制弱，0 为不限制,
	//第三个参数和第四个参数只能设置一个，不可以同时设置
	//第五个参数是限制速度，最大允许调用此接口多大速度。防止延迟太大，0是不限制
	//第六个参数是内部统计从收到请求到调用回调的耗时，会单独启用一个线程来输出日志信息
	//调用超速会阻塞，出口速度跟不上，满了也会阻塞
	int curlHttpCliInit(int threadNum, int maxQueue, int maxPreConns = 0, int maxNewConns = 0, int maxSpeed = 0, int isShowTimeUse = 0);//最后一个参数在调试并发量的时候才会使用

	//程序阻塞，如果malloc失败返回-1
	//如果程序已经收到退出命令返回-2
	//此接口为阻塞的。如果队列大小满了，就会一直阻塞方法中不返回，可以修改为满了就立马退出来
	int curlHttpRequest(HttpReqSession& curlReq);

	////下面方法内部使用，上层应用不可使用，无需关心
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//内部使用，外部使用不需要使用
	void curlHttpClientWakeup();

	void curlHttpThreadReady();

	uint32_t currentConnAdd()
	{
		return currentConn.incrementAndGet();
	}

	uint32_t currentConnDec()
	{
		return currentConn.decrementAndGet();
	}

	uint32_t curlHttpClientMaxConns()
	{
		uint32_t conns = 0;
		if(maxPreConns_ > 0)
		{
			conns = static_cast<uint32_t>(maxPreConns_);
		}
		return conns;
	}
	
private:

	void httpCliIoThread(int index);

	void httpStatisticsSecondThread();

	void httpRspCallBackThread();

	void httpOutRspCallBackThread();

	void curlHttpThreadExit();

	int ioThreadNum;
	int isExit_;
	int isReady;
	int isShowtime;
	int maxPreConns_;
	int maxNewConns_;
	AtomicUInt32 readyNum;
	AtomicUInt32 exitNum;
	AtomicUInt32 lastIndex;
	AtomicUInt32 currentConn;
	std::unique_ptr<ThreadPool> httpCliIoPoolPtr;
	std::unique_ptr<Thread> httpCliCallPtr;
	std::unique_ptr<Thread> httpCliOutCallPtr;
	std::unique_ptr<Thread> httpCliLog;
	std::vector<AsyncCurlHttp*> curlCliVect;
};

}
#endif