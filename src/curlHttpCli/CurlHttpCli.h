#ifndef __XIAO_CURL_HTTP_CLI_H__
#define __XIAO_CURL_HTTP_CLI_H__

#include <mutex>

#include "src/noncopyable.h"
#include "src/Singleton.h"

#include "HttpReqSession.h"


#define ABSOLUTELY_OUT_HTTP_SECOND		180//绝对的超时时间，当速度过快的时候，libcurl和libevent中超时机制可能没有办法工作，这个绝对超时时间会将这些回调调用，防止丢数据
#define MAX_SIMULTANEOUSLY_CONNS		10000//默认最大的连接，绝对不可以超过万连接

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
	//默认构造函数
	CurlHttpCli();

	~CurlHttpCli();

	static CurlHttpCli& instance() { return Singleton<CurlHttpCli>::instance();}

	//程序退出时调用方法，会清楚所有异步数据之后退出，阻塞方法，返回则表示数据已经清除了
	void curlHttpCliExit();

	//第一个参数为IO线程数，内部会启动1个线程，回调在另外一个线程处理,同时处理超时无响应的连接，建议 2
	//第二个参数是最大的队列大小，内部缓冲队列大小，请求发起时会放入一个队列，如果为 0，不限制大小。如果比较大，而且入口大于出口速度，响应延迟可能就会很大
	//第三个参数是是否保持长连接，如果这个模块只请求一个URL,才可以设置为保持长连接，如果需要请求多个URL,这个参数必须为0
	//第四个参数是同时并发最大的维持连接池的大小。默认最大不能超过10000，
	//第五个参数是限速，调用此接口不可以超过这个速度，限制速度使用(暂时还没有使用)
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
