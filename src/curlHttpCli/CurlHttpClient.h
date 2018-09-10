
#include <sys/eventfd.h>

#include "Incommon.h"
#include "HttpRequestQ.h"
#include "CurlHttpClient.h"

namespace HTTPCLI
{

static int createEventfd()
{
	int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (evtfd < 0)
	{
		HTTPCLI_LOG_WARN("Failed in eventfd");
	}
	return evtfd;
}

static void wakeUpFdcb(int sockFd, short eventType, void *arg)
{
	CurlHttpClient *p = static_cast<CurlHttpClient*>(arg);
	p->handleRead();
}

static void timeUpFdcb(int sockFd, short eventType, void *arg)
{
	CurlHttpClient *p = static_cast<CurlHttpClient*>(arg);
	p->timeExpireCb();
}

/* CURLMOPT_SOCKETFUNCTION */
static int sockFdcb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp)
{
	CurlHttpClient *p = static_cast<CurlHttpClient*>(cbp);
	p->curlSockFdCb(e, s, what, sockp);
	return 0;
}

/* Update the event timer after curl_multi library calls */
static int multiTimerCb(CURLM *multi, long timeout_ms, void *arg)
{
	CurlHttpClient *p = static_cast<CurlHttpClient*>(arg);
	p->culrMultiTimerCb(timeout_ms);
	return 0;
}

static void eventFdcb(int fd, short kind, void *arg)
{
	CurlHttpClient *p = static_cast<CurlHttpClient*>(arg);
	p->curlEventFdcb(fd, kind);
}

/* CURLOPT_WRITEFUNCTION */
static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	ConnInfo *conn = static_cast<ConnInfo*>(data);
	HTTPCLI_LOG_DEBUG("write_cb: %s (%ld/%ld)", conn->url, size, nmemb);
	if(conn->rspData)
	{
		size_t needLen = strlen(conn->rspData);
		needLen += realsize;
		char* tmpP = static_cast<char*>(malloc(needLen));
		if(tmpP)
		{
			memset(tmpP, 0 , needLen);
			needLen = strlen(conn->rspData) - 1;
			memcpy(tmpP, conn->rspData, needLen);
			tmpP += needLen;
			memcpy(tmpP, ptr, realsize);
			free(conn->rspData);
			conn->rspData = tmpP;
		}
	}else{
		size_t needLen = realsize + 1;
		char* tmpP = static_cast<char*>(malloc(needLen));
		if(tmpP)
		{
			memset(tmpP, 0 , needLen);
			memcpy(tmpP, ptr, realsize);
			conn->rspData = tmpP;
		}
	}
	return realsize;
}

/* CURLOPT_PROGRESSFUNCTION */
static int prog_cb(void *p, double dltotal, double dlnow, double ult, double uln)
{
	//ConnInfo *conn = static_cast<ConnInfo *>(p);

	//HTTPCLI_LOG_DEBUG("prog_cb: %s (%g/%g)", conn->url, dlnow, dltotal);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CurlHttpClient::CurlHttpClient()
	:isRun(1)
	,wakeupFd_(createEventfd())
	,gInfo_(nullptr)
{
	PDEBUG("CurlHttpClient init");
}

CurlHttpClient::~CurlHttpClient()
{
	PERROR("~CurlHttpClient exit");
}

int CurlHttpClient::curlHttpClientReady()
{
	HTTPCLI_LOG_INFO("curlHttpClientReady in");
	if(wakeupFd_ < 0)
	{
		HTTPCLI_LOG_WARN("curlHttpClientReady wakeupFd_ error %d", wakeupFd_);
		return wakeupFd_;
	}

	gInfo_.reset(new GlobalInfo());
	if(gInfo_ == nullptr)
	{
		HTTPCLI_LOG_WARN("curlHttpClientReady GlobalInfo new error");
		return -1;
	}

	gInfo_->evbase = event_base_new();
	if(gInfo_->evbase == NULL)
	{
		HTTPCLI_LOG_WARN("curlHttpClientReady event_base_new new error");
		return -1;
	}
	
	event_assign(&gInfo_->wake_event, gInfo_->evbase, wakeupFd_, EV_READ|EV_PERSIST, wakeUpFdcb, this);
	event_add(&gInfo_->wake_event, NULL);

	gInfo_->multi = curl_multi_init();
  	evtimer_assign(&gInfo_->timer_event, gInfo_->evbase, timeUpFdcb, this);

	/* setup the generic multi interface options we want */
	curl_multi_setopt(gInfo_->multi, CURLMOPT_SOCKETFUNCTION, sockFdcb);
	curl_multi_setopt(gInfo_->multi, CURLMOPT_SOCKETDATA, this);
	curl_multi_setopt(gInfo_->multi, CURLMOPT_TIMERFUNCTION, multiTimerCb);
	curl_multi_setopt(gInfo_->multi, CURLMOPT_TIMERDATA, this);

	/* we don't call any curl_multi_socket*() function yet as we have no handles
     added! */
	event_base_dispatch(gInfo_->evbase);
	HTTPCLI_LOG_WARN("event_base_dispatch return");

	event_del(&gInfo_->wake_event);
	close(wakeupFd_);
	event_del(&gInfo_->timer_event);
	event_base_free(gInfo_->evbase);
	curl_multi_cleanup(gInfo_->multi);
	return 0;
}

void CurlHttpClient::wakeup()
{
	uint64_t one = 1;
	ssize_t n = write(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
	{
		HTTPCLI_LOG_WARN("EventLoop::wakeup() writes %ld bytes instead of 8", n);
	}
}

void CurlHttpClient::handleRead()
{
	uint64_t one = 1;
	ssize_t n = read(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
	{
		HTTPCLI_LOG_WARN("EventLoop::handleRead() reads %ld bytes instead of 8", n);
		return;
	}

	if(one == 4)
	{
		gInfo_->stopped = 1;
		if(gInfo_->still_running == 0)
			event_base_loopbreak(gInfo_->evbase);
		
		return;
	}
	
	requetHttpServer();
}

void CurlHttpClient::timeExpireCb()
{
	CURLMcode rc;
	rc = curl_multi_socket_action(gInfo_->multi,
                                  CURL_SOCKET_TIMEOUT, 0, &gInfo_->still_running);
	
	mcode_or_die("timer_cb: curl_multi_socket_action", rc);
	check_multi_info();
}

void CurlHttpClient::curlSockFdCb(CURL *e, curl_socket_t s, int what, void *sockp)
{
	SockInfo *fdp = static_cast<SockInfo*>(sockp);
	const char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE" };

	HTTPCLI_LOG_DEBUG("socket callback: s=%d e=%p what=%s ", s, e, whatstr[what]);
	if(what == CURL_POLL_REMOVE)
	{
		HTTPCLI_LOG_DEBUG("CURL_POLL_REMOVE");
		remsock(fdp);
	}
	else
	{
		if(!fdp)
		{
			HTTPCLI_LOG_DEBUG("Adding data: %s", whatstr[what]);
			addsock(s, e, what, gInfo_.get());
		}
		else
		{
			HTTPCLI_LOG_DEBUG("Changing action from %s to %s",
			      whatstr[fdp->action], whatstr[what]);
			setsock(fdp, s, e, what, gInfo_.get());
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CurlHttpClient::requetHttpServer()
{
	ConnInfo *conn;
	CURLMcode rc;

	CurlHttpRequestPtr reqInfo = HTTPCLI::HttpRequestQ::instance().dealRequest();
	if(reqInfo == nullptr)
		return;
	
	//conn = static_cast<ConnInfo *>(malloc(sizeof(ConnInfo)));
	//memset(&conn, 0, sizeof (ConnInfo));
	conn =  static_cast<ConnInfo *>(calloc(1, sizeof(ConnInfo)));
	if(conn == NULL)
	{
		HTTPCLI_LOG_WARN("requetHttpServer new ConnInfo error");
		reqInfo->setCurlRspCode(-1);
		reqInfo->curlRespondCallBack(reqInfo);
		return;
	}
	conn->error[0]='\0';

	conn->easy = curl_easy_init();
	if(!conn->easy)
	{
		HTTPCLI_LOG_WARN("curl_easy_init() failed, exiting!\n");
		reqInfo->setCurlRspCode(-1);
		reqInfo->curlRespondCallBack(reqInfo);
		free(conn);
		return;
	}
	conn->global = gInfo_.get();

	std::string url = reqInfo->curlHttpUrl();
	conn->url = static_cast<char *>(malloc((url.size() + 1)));
	if(conn->url == NULL)
	{
		HTTPCLI_LOG_WARN("curl_easy_init() failed, exiting!\n");
		reqInfo->setCurlRspCode(-1);
		reqInfo->curlRespondCallBack(reqInfo);
		free(conn);
		return;
	}
	memset(conn->url, 0, (url.size() + 1));
	memcpy(conn->url, url.c_str(), url.size());

	//https
	if(reqInfo->curlHttpVersion() == HTTPS)
	{
		HTTPCLI_LOG_WARN("curl_easy_init https request");
		curl_easy_setopt(conn->easy, CURLOPT_SSL_VERIFYPEER, true);
    	curl_easy_setopt(conn->easy, CURLOPT_SSL_VERIFYHOST, true);
		curl_easy_setopt(conn->easy, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(conn->easy, CURLOPT_AUTOREFERER, 1);
    	curl_easy_setopt(conn->easy, CURLOPT_CAINFO, "./cacert.pem");
	}

	switch(reqInfo->curlHttpReqType())
	{
		case HTTP_GET:
			curl_easy_setopt(conn->easy, CURLOPT_HTTPGET, 1);
			break;
		case HTTP_POST:
			curl_easy_setopt(conn->easy, CURLOPT_POST, 1L);
			break;
		case HTTP_PUT:
			curl_easy_setopt(conn->easy, CURLOPT_CUSTOMREQUEST, "PUT");
			break;
		case HTTP_DELETE:
			curl_easy_setopt(conn->easy, CURLOPT_CUSTOMREQUEST, "DELETE");
			break;
		case HTTP_UPDATE:
			curl_easy_setopt(conn->easy, CURLOPT_CUSTOMREQUEST, "UPDATE");
			break;
		default:
			HTTPCLI_LOG_WARN("reqInfo->curlHttpReqType %d no support now", reqInfo->curlHttpReqType());
			reqInfo->setCurlRspCode(-2);
			reqInfo->curlRespondCallBack(reqInfo);
			free(conn);
			return;
	}

	std::string bodyData = reqInfo->curlHttpData();
	if(!bodyData.empty())
	{
		HTTPCLI_LOG_WARN("curl_easy_init set http body");
		curl_easy_setopt(conn->easy, CURLOPT_POSTFIELDSIZE, bodyData.length());
    	curl_easy_setopt(conn->easy, CURLOPT_POSTFIELDS, bodyData.c_str());
	}

	char headBuf[1024] = {0};
	#ifdef XIAOMI_HTTP_TYPE
	if(reqInfo->curlHttpType() == 0)
	{
		//struct curl_slist *headers=NULL; /* init to NULL is important */
		//char headBuf[1024] = {0};
		memset(headBuf, 0, 1024);
		sprintf(headBuf,"appId:%ld",reqInfo->curlRspAppId());
		//HTTPCLI_LOG_WARN("headers %s ", headBuf);
		conn->headers = curl_slist_append(conn->headers, headBuf);
		
		memset(headBuf, 0, 1024);
		sprintf(headBuf,"appKey:%s",reqInfo->curlRspAppkey().c_str());
		//HTTPCLI_LOG_WARN("headers %s ", headBuf);
		conn->headers = curl_slist_append(conn->headers, headBuf);
		
		memset(headBuf, 0, 1024);
		sprintf(headBuf,"appSecret:%s",reqInfo->curlRspAppSecret().c_str());
		//HTTPCLI_LOG_WARN("headers %s ", headBuf);
		conn->headers = curl_slist_append(conn->headers, headBuf);

		conn->headers = curl_slist_append(conn->headers, "Content-Type: application/json;charset=UTF-8");
		conn->headers = curl_slist_append(conn->headers, "Accept:application/json;charset=UTF-8");

		/* pass our list of custom made headers */
		//curl_easy_setopt(conn->easy, CURLOPT_HTTPHEADER, conn->headers);
		
		//curl_slist_free_all(headers); /* free the header list *///no free herr will be core
	}
	#endif

	HttpHeadPrivate headV = reqInfo->curlHttpHead();
	for(size_t i = 0; i < headV.size(); i++)
	{
		memset(headBuf, 0, 1024);
		sprintf(headBuf,"%s", headV[i].c_str());
		HTTPCLI_LOG_WARN("headers %s ", headBuf);
		conn->headers = curl_slist_append(conn->headers, headBuf);
	}

	if(conn->headers)
	{
		curl_easy_setopt(conn->easy, CURLOPT_HTTPHEADER, conn->headers);
	}

	curl_easy_setopt(conn->easy, CURLOPT_URL, conn->url);
	curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, conn);
	curl_easy_setopt(conn->easy, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(conn->easy, CURLOPT_ERRORBUFFER, conn->error);
	curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);
	curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(conn->easy, CURLOPT_PROGRESSFUNCTION, prog_cb);
	curl_easy_setopt(conn->easy, CURLOPT_PROGRESSDATA, conn);
	curl_easy_setopt(conn->easy, CURLOPT_FOLLOWLOCATION, 1L);

	conn->reqInfo = reqInfo;
	HTTPCLI_LOG_DEBUG("Adding easy %p to multi %p (%s)", conn->easy, gInfo_->multi, url.c_str());
	rc = curl_multi_add_handle(gInfo_->multi, conn->easy);
	mcode_or_die("new_conn: curl_multi_add_handle", rc);

	/* note that the add_handle() will set a time-out to trigger very soon so
	 	that the necessary socket_action() call will be called by this app */
}

void CurlHttpClient::mcode_or_die(const char *where, CURLMcode code)
{
	if(CURLM_OK != code)
	{
		const char *s = "Hello";
		switch(code)
		{
			case CURLM_BAD_HANDLE:
				break;
			case CURLM_BAD_EASY_HANDLE:
				break;
			case CURLM_OUT_OF_MEMORY:
				break;
			case CURLM_INTERNAL_ERROR:
				break;
			case CURLM_UNKNOWN_OPTION:
				break;
			case CURLM_LAST:
				break;
			default: 
				s = "CURLM_unknown";
				break;
			case CURLM_BAD_SOCKET:
				HTTPCLI_LOG_WARN( "ERROR: %s returns %s\n", where, s);
				/* ignore this error */
			return;
		}
		HTTPCLI_LOG_WARN("ERROR: %s returns %s\n", where, s);
		//exit(code);
	}
}

/* Check for completed transfers, and remove their easy handles */
void CurlHttpClient::check_multi_info()
{
	char *eff_url;
	CURLMsg *msg;
	int msgs_left;
	ConnInfo *conn;
	CURL *easy;
	CURLcode res;

	HTTPCLI_LOG_DEBUG("REMAINING: %d", gInfo_->still_running);
	while((msg = curl_multi_info_read(gInfo_->multi, &msgs_left)))
	{
		if(msg->msg == CURLMSG_DONE)
		{
			long response_code;
			easy = msg->easy_handle;
			res = msg->data.result;
			curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
			curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
			curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response_code);
			HTTPCLI_LOG_DEBUG("DONE: %s => (%d) %ld %s", eff_url, res, response_code, conn->error);
			//curl_easy_getinfo(easy, CURLINFO_REQUEST_SIZE, &response_code);
			//HTTPCLI_LOG_DEBUG("DONE: %s => (%d) %ld %s", eff_url, res, response_code, conn->error);
			//HTTPCLI_LOG_DEBUG("DONE: %s => (%d) %ld %s", eff_url, res, response_code, conn->rspData);

			conn->reqInfo->setCurlRspCode(static_cast<int>(response_code));
			if(conn->rspData)
			{
				conn->reqInfo->setCurlRspBody(conn->rspData);
			}
			conn->reqInfo->curlRespondCallBack(conn->reqInfo);
			curl_multi_remove_handle(gInfo_->multi, easy);
			free(conn->url);
			if(conn->rspData)
			{
				free(conn->rspData);
			}
			if(conn->headers)
			{
				curl_slist_free_all(conn->headers);
			}
			curl_easy_cleanup(easy);
			free(conn);
		}
	}
	
	if(gInfo_->still_running == 0 && gInfo_->stopped)
		event_base_loopbreak(gInfo_->evbase);
}

/* Initialize a new SockInfo structure */
void CurlHttpClient::addsock(curl_socket_t s, CURL *easy, int action, GlobalInfo *g)
{
	SockInfo *fdp = static_cast<SockInfo *>(calloc(sizeof(SockInfo), 1));
	//SockInfo *fdp = static_cast<SockInfo *>(malloc(sizeof(SockInfo)));
	//memset(fdp , 0 ,sizeof(SockInfo));
	
	fdp->global = g;
	setsock(fdp, s, easy, action, g);
	curl_multi_assign(gInfo_->multi, s, fdp);
}

/* Assign information to a SockInfo structure */
void CurlHttpClient::setsock(SockInfo *f, curl_socket_t s, CURL *e, int act, GlobalInfo *g)
{
	short kind = static_cast<short>((act&CURL_POLL_IN?EV_READ:0)|(act&CURL_POLL_OUT?EV_WRITE:0)|EV_PERSIST);

	f->sockfd = s;
	f->action = act;
	f->easy = e;
	event_del(&f->ev);
	event_assign(&f->ev, gInfo_->evbase, f->sockfd, kind, eventFdcb, this);
	event_add(&f->ev, NULL);
}

/* Clean up the SockInfo structure */
void CurlHttpClient::remsock(SockInfo *f)
{
	if(f)
	{
		event_del(&f->ev);
		free(f);
	}
}

/* Called by libevent when we get action on a multi socket */
void CurlHttpClient::curlEventFdcb(int fd, short kind)
{
	CURLMcode rc;

	int action =
		(kind & EV_READ ? CURL_CSELECT_IN : 0) |
		(kind & EV_WRITE ? CURL_CSELECT_OUT : 0);

	rc = curl_multi_socket_action(gInfo_->multi, fd, action, &gInfo_->still_running);
	mcode_or_die("event_cb: curl_multi_socket_action", rc);

	check_multi_info();
	if(gInfo_->still_running <= 0)
	{
		HTTPCLI_LOG_DEBUG("last transfer done, kill timeout");
		if(evtimer_pending(&gInfo_->timer_event, NULL))
		{
			evtimer_del(&gInfo_->timer_event);
		}
	}
}

void CurlHttpClient::culrMultiTimerCb(long timeout_ms)
{
	struct timeval timeout;
	CURLMcode rc;

	timeout.tv_sec = timeout_ms/1000;
	timeout.tv_usec = (timeout_ms%1000)*1000;
	HTTPCLI_LOG_DEBUG("multi_timer_cb: Setting timeout to %ld ms", timeout_ms);

	/* TODO
	*
	* if timeout_ms is 0, call curl_multi_socket_action() at once!
	*
	* if timeout_ms is -1, just delete the timer
	*
	* for all other values of timeout_ms, this should set or *update*
	* the timer to the new value
	*/
	if(timeout_ms == 0) {
		rc = curl_multi_socket_action(gInfo_->multi, CURL_SOCKET_TIMEOUT, 0, &gInfo_->still_running);
		mcode_or_die("multi_timer_cb: curl_multi_socket_action", rc);
	}
	else if(timeout_ms == -1)
		evtimer_del(&gInfo_->timer_event);
	else
		evtimer_add(&gInfo_->timer_event, &timeout);
}




}
