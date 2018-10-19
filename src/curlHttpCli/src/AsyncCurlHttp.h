#ifndef __XIAO_ASYNC_CURL_HTTP_H__
#define __XIAO_ASYNC_CURL_HTTP_H__

#include <memory>

#include <curl/curl.h>
#include <event2/event.h>
#include <event2/event_struct.h>

#include "../HttpReqSession.h"

#define CURL_ERROR_SIZE 256
namespace CURL_HTTP_CLI
{

/* Global information, common to all connections */
typedef struct _GlobalInfo
{
	struct event_base *evbase;
	struct event wake_event;
	struct event timer_event;
	CURLM *multi;
	int still_running;
	int stopped;
} GlobalInfo;

/* Information associated with a specific easy handle */
typedef struct _ConnInfo
{
	CURL *easy;
	char *url;
	GlobalInfo *global;
	struct curl_slist *headers;
	HttpReqSession* reqInfo;
	char *rspData;
	char error[CURL_ERROR_SIZE];
} ConnInfo;

/* Information associated with a specific socket */
typedef struct _SockInfo
{
	curl_socket_t sockfd;
	CURL *easy;
	int action;
	long timeout;
	struct event ev;
	GlobalInfo *global;
} SockInfo;

int prog_cb(void *p, double dltotal, double dlnow, double ult, double uln);

size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data);

class AsyncCurlHttp
{
public:
	//参数是否计算时间耗时
	AsyncCurlHttp(int maxPreSize_);
	~AsyncCurlHttp();

	int curlHttpClientReady();

	void wakeup();

	void asyncCurlExit();

	void handleRead();

	void timeExpireCb();

	void curlSockFdCb(CURL *e, curl_socket_t s, int what, void *sockp);

	void culrMultiTimerCb(long timeout_ms);

	void curlEventFdcb(int fd, short kind);
private:
	void requetHttpServer();

	void requetHttpServer(ConnInfo* conn);
	
	void mcode_or_die(const char *where, CURLMcode code);

	void check_multi_info();

	void addsock(curl_socket_t s, CURL *easy, int action, GlobalInfo *g);

	void setsock(SockInfo *f, curl_socket_t s, CURL *e, int act, GlobalInfo *g);
	
	void remsock(SockInfo *f);
	
	int isRun;
	int wakeupFd_;
	int maxPreSize;
	volatile int queueSize;
	GlobalInfo *gInfo_;
};

//typedef std::shared_ptr<AsyncCurlHttp> AsyncCurlHttpPtr;
}
#endif