#ifndef __XIAO_ASYNC_CURL_HTTP_H__
#define __XIAO_ASYNC_CURL_HTTP_H__

#include <curl/curl.h>
#include <event2/event.h>
#include <event2/event_struct.h>

#include "Atomic.h"

#include "../HttpReqSession.h"
#include "../CurlHttpCli.h"

namespace CURL_HTTP_CLI
{

class AsyncCurlHttp
{
public:
	AsyncCurlHttp(size_t subIndex, size_t maxqueue);
	
	~AsyncCurlHttp();

	int asyncCurlReady();

	void wakeup();

	void asyncCurlExit();

	void handleRead();

	void timeExpireCb();

	void curlSockFdCb(CURL *e, curl_socket_t s, int what, void *sockp);

	void culrMultiTimerCb(long timeout_ms);

	void curlEventFdcb(int fd, short kind);

	int curlhttpRequest(HttpReqSession* req);

private:
	
	void requetHttpServer();

	void requetHttpServer(ConnInfo* conn, HttpReqSession* sess);

	int mcode_or_die(const char *where, CURLMcode code);

	void check_multi_info();

	void addsock(curl_socket_t s, CURL *easy, int action, GlobalInfo *g);

	void setsock(SockInfo *f, curl_socket_t s, CURL *e, int act, GlobalInfo *g);
	
	void remsock(SockInfo *f);

	int isExit;
	int wakeupFd_;
	
	volatile size_t queueSize;
	size_t ioThreadIndex;
	size_t maxQueueSize_;
	
	GlobalInfo *gInfo_;
	HttpRequestQueuePtr reqQueuePtr;
	HttpConnInfoVectorPtr connVectPtr;
};

typedef std::shared_ptr<AsyncCurlHttp> AsyncCurlHttpPtr;

}
#endif
