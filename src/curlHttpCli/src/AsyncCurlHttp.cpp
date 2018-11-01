
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/cdefs.h>

#include "Incommon.h"
#include "ConnInfo.h"
#include "HttpCliQueue.h"
#include "AsyncCurlHttp.h"


#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace CURL_HTTP_CLI
{

static int createEventfd()
{
	int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (evtfd < 0)
	{
		WARN("Failed in eventfd");
	}
	return evtfd;
}

static void wakeUpFdcb(int sockFd, short eventType, void *arg)
{
	AsyncCurlHttp *p = (AsyncCurlHttp *)arg;
	p->handleRead();
}

static void timeUpFdcb(int sockFd, short eventType, void *arg)
{
	AsyncCurlHttp *p = (AsyncCurlHttp *)arg;
	p->timeExpireCb();
}

/* CURLMOPT_SOCKETFUNCTION */
static int sockFdcb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp)
{
	AsyncCurlHttp *p = (AsyncCurlHttp *)cbp;
	p->curlSockFdCb(e, s, what, sockp);
	return 0;
}

/* Update the event timer after curl_multi library calls */
static int multiTimerCb(CURLM *multi, long timeout_ms, void *arg)
{
	AsyncCurlHttp *p = (AsyncCurlHttp *)arg;
	p->culrMultiTimerCb(timeout_ms);
	return 0;
}

static void eventFdcb(int fd, short kind, void *arg)
{
	AsyncCurlHttp *p = (AsyncCurlHttp *)arg;
	p->curlEventFdcb(fd, kind);
}

/* CURLOPT_WRITEFUNCTION */
#if 0
static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
	char *pData = (char *)ptr;
	size_t realsize = size * nmemb;
	ConnInfo *conn = (ConnInfo*)data;
	//DEBUG("write_cb: %s (%ld/%ld) %s", conn->url, size, nmemb, pData);

	if(conn->reqInfo->httpResponstCode() == 0)
	{
		if(conn->rspData)
		{
			size_t needLen = strlen(conn->rspData);
			needLen += realsize + 1;
			//DEBUG("conn->rspData %p %ld", conn->rspData, needLen);
			char* tmpP = (char*)malloc(needLen);
			if(tmpP)
			{
				pData = tmpP;
				memset(tmpP, 0 , needLen);
				needLen = strlen(conn->rspData);
				memcpy(tmpP, conn->rspData, needLen);
				tmpP += needLen;
				memcpy(tmpP, ptr, realsize);
				free(conn->rspData);
				conn->rspData = pData;
			}else{
				free(conn->rspData);
				conn->rspData = NULL;
				conn->reqInfo->setHttpResponseCode(-1);
				conn->reqInfo->setHttpReqErrorMsg("malloc null");
			}
		}else{
			size_t needLen = realsize + 1;
			//DEBUG("conn->rspData NULL %ld", needLen);
			char* tmpP = (char*)malloc(needLen);
			if(tmpP)
			{
				memset(tmpP, 0 , needLen);
				memcpy(tmpP, ptr, realsize);
				conn->rspData = tmpP;
			}else{
				conn->reqInfo->setHttpResponseCode(-1);
				conn->reqInfo->setHttpReqErrorMsg("malloc null");
			}
		}
	}
	return realsize;
}
#else
static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
	char *pData = (char *)ptr;
	size_t realsize = size * nmemb;
	ConnInfo *conn = (ConnInfo*)data;
	//DEBUG("write_cb: %s (%ld/%ld) %s", conn->url, size, nmemb, pData);

	if(conn->connInfoReqinfo() && conn->connInfoReqinfo()->httpResponstCode() == 0)
	{
		if(conn->connInfoRspBody().empty())
		{
			conn->connInfoSetRspbody(pData, realsize);
		}else{
			conn->connInfoAppendRspbody(pData, realsize);
		}
	}
	
	return realsize;
}

#endif

/* CURLOPT_PROGRESSFUNCTION */
static int prog_cb(void *p, double dltotal, double dlnow, double ult, double uln)
{
	//ConnInfo *conn = static_cast<ConnInfo *>(p);

	//HTTPCLI_LOG_DEBUG("prog_cb: %s (%g/%g)", conn->url, dlnow, dltotal);
	return 0;
}

AsyncCurlHttp::AsyncCurlHttp(size_t maxConns)
	:isExit(0)
	,wakeupFd_(createEventfd())
	,queueSize(0)
	,maxConnSize(maxConns)
	,gInfo_(nullptr)
	,connVectPtr(nullptr)
	,connQueuePtr(nullptr)
{
	DEBUG("CurlHttpClient init");
}

AsyncCurlHttp::~AsyncCurlHttp()
{
	if(gInfo_)
	{
		free(gInfo_);
	}

	if(connVectPtr)
	{
		connVectPtr.reset();
	}

	if(connQueuePtr)
	{
		connQueuePtr.reset();
	}
	ERROR("~CurlHttpClient exit");
}

int AsyncCurlHttp::asyncCurlReady()
{
	INFO("curlHttpClientReady in");
	if(wakeupFd_ < 0)
	{
		WARN("curlHttpClientReady wakeupFd_ error %d", wakeupFd_);
		return -1;
	}
	
	connVectPtr.reset(new HttpConnInfoVector());
	if(connVectPtr == nullptr)
	{
		WARN("curlHttpClientReady connVectPtr new error");
		return -1;
	}
	connVectPtr->setMaxVectorSize(maxConnSize);

	connQueuePtr.reset(new HttpConnInfoQueue());
	if(connQueuePtr == nullptr)
	{
		WARN("curlHttpClientReady connQueuePtr new error");
		return -1;
	}
	
	gInfo_ =  (GlobalInfo *)malloc(sizeof(GlobalInfo));
	if(gInfo_ == nullptr)
	{
		WARN("curlHttpClientReady GlobalInfo new error");
		return -1;
	}
	memset(gInfo_, 0, sizeof (GlobalInfo));

	gInfo_->evbase = event_base_new();
	if(gInfo_->evbase == NULL)
	{
		WARN("curlHttpClientReady event_base_new new error");
		return -1;
	}
	
	event_assign(&gInfo_->wake_event, gInfo_->evbase, wakeupFd_, EV_READ|EV_PERSIST, wakeUpFdcb, this);
	event_add(&gInfo_->wake_event, NULL);

	gInfo_->multi = curl_multi_init();
  	evtimer_assign(&gInfo_->timer_event, gInfo_->evbase, timeUpFdcb, this);

	curl_multi_setopt(gInfo_->multi, CURLMOPT_MAX_TOTAL_CONNECTIONS, 60000 );
	curl_multi_setopt(gInfo_->multi, CURLMOPT_MAX_HOST_CONNECTIONS, 10000 );
	curl_multi_setopt(gInfo_->multi, CURLMOPT_MAXCONNECTS, 60000);
	//curl_multi_setopt(gInfo_->multi, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
	//curl_multi_setopt(gInfo_->multi, CURLMOPT_MAXCONNECTS, (10000));
	//curl_multi_setopt(gInfo_->multi, CURLMOPT_MAXCONNECTS, 60000);
	
	/* setup the generic multi interface options we want */
	curl_multi_setopt(gInfo_->multi, CURLMOPT_SOCKETFUNCTION, sockFdcb);
	curl_multi_setopt(gInfo_->multi, CURLMOPT_SOCKETDATA, this);
	curl_multi_setopt(gInfo_->multi, CURLMOPT_TIMERFUNCTION, multiTimerCb);
	curl_multi_setopt(gInfo_->multi, CURLMOPT_TIMERDATA, this);

	/* we don't call any curl_multi_socket*() function yet as we have no handles
     added! */
    CurlHttpCli::instance().curlHttpThreadReady();
	event_base_dispatch(gInfo_->evbase);
	WARN("event_base_dispatch return %p", gInfo_->evbase);

	event_del(&gInfo_->wake_event);
	close(wakeupFd_);
	event_del(&gInfo_->timer_event);
	event_base_free(gInfo_->evbase);
	curl_multi_cleanup(gInfo_->multi);
	return 0;
}

void AsyncCurlHttp::wakeup()
{
	uint64_t one = 2;
	ssize_t n = write(wakeupFd_, &one, sizeof one);
	//INFO("wakeup n one %ld %ld %p", n, one, gInfo_->evbase);
	if (n != sizeof one)
	{
		WARN("EventLoop::wakeup() writes %ld bytes instead of 8", n);
	}
}

void AsyncCurlHttp::asyncCurlExit()
{
	uint64_t one = 1;
	ssize_t n = write(wakeupFd_, &one, sizeof one);
	INFO("curlHttpClientExit n one %ld %ld %p", n, one, gInfo_->evbase);
	if (n != sizeof one)
	{
		WARN("EventLoop::wakeup() writes %ld bytes instead of 8", n);
	}
}

void AsyncCurlHttp::handleRead()
{
	uint64_t one = 1;
	ssize_t n = read(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
	{
		WARN("EventLoop::handleRead() reads %ld bytes instead of 8", n);
		return;
	}
	//INFO("handleRead n one %ld %ld :: %p", n, one, gInfo_->evbase);
	uint64_t num = one / 2;
	queueSize += static_cast<size_t>(num);

	requetHttpServer();
	
	num = one % 2;
	if(num || gInfo_->stopped == 1)
	{
		gInfo_->stopped = 1;
		if(gInfo_->still_running == 0)
		{
			event_base_loopbreak(gInfo_->evbase);
		}
	}
}

void AsyncCurlHttp::timeExpireCb()
{
	CURLMcode rc;
	rc = curl_multi_socket_action(gInfo_->multi, CURL_SOCKET_TIMEOUT, 0, &gInfo_->still_running);
	
	mcode_or_die("timer_cb: curl_multi_socket_action", rc);
	check_multi_info();
}

void AsyncCurlHttp::curlSockFdCb(CURL *e, curl_socket_t s, int what, void *sockp)
{
	SockInfo *fdp = static_cast<SockInfo*>(sockp);
	//const char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE" };

	//DEBUG("socket callback: s=%d e=%p what=%s ", s, e, whatstr[what]);
	if(what == CURL_POLL_REMOVE)
	{
		//DEBUG("CURL_POLL_REMOVE");
		remsock(fdp);
	}
	else
	{
		if(!fdp)
		{
			//DEBUG("Adding data: %s", whatstr[what]);
			addsock(s, e, what, gInfo_);
		}
		else
		{
			//DEBUG("Changing action from %s to %s", whatstr[fdp->action], whatstr[what]);
			setsock(fdp, s, e, what, gInfo_);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
void AsyncCurlHttp::requetHttpServer()
{
	size_t reqNum = queueSize;
	for(size_t i = 0; i < reqNum; i++)
	{
		ConnInfo* conn = connQueuePtr->httpcliConnPop();
		if(conn == NULL && connVectPtr->isFull())
		{
			break;
		}
		if(queueSize)
		{
			queueSize--;
		}
		HttpReqSession* req = CURL_HTTP_CLI::HttpRequestQueue::instance().dealRequest();
		if(req)
		{
			requetHttpServer(conn, req);
		}else{
			if(conn)
			{
				connQueuePtr->httpcliConnInsert(conn);
			}
			break;
		}
	}

	connVectPtr->httpcliConnForEach(connQueuePtr);
}

void AsyncCurlHttp::requetHttpServer(ConnInfo* conn, HttpReqSession* sess)
{
	int ret = 0;
	CURLcode tRetCode = CURLE_OK;
	if(conn == NULL)
	{
		conn = new ConnInfo();
		if(conn == NULL)
		{
			WARN("requetHttpServer new ConnInfo error");
			sess->setHttpResponseCode(-1);
			sess->setHttpReqErrorMsg("malloc connInfo nullptr");
			CURL_HTTP_CLI::HttpResponseQueue::instance().httpResponse(sess);
			return;
		}
		
		ret = conn->connInfoInit();
		if(ret < 0)
		{
			WARN("requetHttpServer ConnInfo connInfoInit error");
			sess->setHttpResponseCode(-1);
			sess->setHttpReqErrorMsg("connInfo init error");
			CURL_HTTP_CLI::HttpResponseQueue::instance().httpResponse(sess);
			delete conn;
			return;
		}

		ret = conn->connInfoSetReqUrl(sess->httpRequestUrl());
		if(ret < 0)
		{
			WARN("requetHttpServer new request url error");
			sess->setHttpResponseCode(-1);
			sess->setHttpReqErrorMsg("connInfo req url error");
			CURL_HTTP_CLI::HttpResponseQueue::instance().httpResponse(sess);
			delete conn;
			return;
		}

		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_TIMEOUT, sess->httpReqDataoutSecond());
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_TIMEOUT failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		
		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_CONNECTTIMEOUT, sess->httpReqConnoutSecond());
	    if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_CONNECTTIMEOUT failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_NOSIGNAL, 1);
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_NOSIGNAL failed!err:%s", curl_easy_strerror(tRetCode));
	    }

		if(CURL_HTTP_CLI::CurlHttpCli::instance().httpIsKeepAlive() == 0)
		{
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_FRESH_CONNECT, 1);
			if (CURLE_OK != tRetCode)
		    {
		        WARN("curl_easy_setopt CURLOPT_FRESH_CONNECT failed!err:%s", curl_easy_strerror(tRetCode));
		    }

			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_FORBID_REUSE, 1L);
			if (CURLE_OK != tRetCode)
		    {
		        WARN("curl_easy_setopt CURLOPT_FORBID_REUSE failed!err:%s", curl_easy_strerror(tRetCode));
		    }
		}else{
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_TCP_KEEPALIVE, 1L);
			if (CURLE_OK != tRetCode)
		    {
		        WARN("curl_easy_setopt CURLOPT_TCP_KEEPALIVE failed!err:%s", curl_easy_strerror(tRetCode));
		    }
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_TCP_KEEPIDLE, 20L);
			if (CURLE_OK != tRetCode)
		    {
		        WARN("curl_easy_setopt CURLOPT_TCP_KEEPIDLE failed!err:%s", curl_easy_strerror(tRetCode));
		    }
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_TCP_KEEPINTVL, 10L);
			if (CURLE_OK != tRetCode)
		    {
		        WARN("curl_easy_setopt CURLOPT_TCP_KEEPINTVL failed!err:%s", curl_easy_strerror(tRetCode));
		    }
		}

		if(sess->httpsSslVerifyPeer())
		{
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_SSL_VERIFYPEER, 1L);
		}else{
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_SSL_VERIFYPEER, 0L);
		}
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_SSL_VERIFYPEER failed!err:%s", curl_easy_strerror(tRetCode));
	    }

		if(sess->httpsSslVerifyHost())
		{
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_SSL_VERIFYHOST, 1L);
		}else{
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_SSL_VERIFYHOST, 0L);
		}
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_SSL_VERIFYHOST failed!err:%s", curl_easy_strerror(tRetCode));
	    }

		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_AUTOREFERER, 1);
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_AUTOREFERER failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		
    	if(sess->httpsCacerFile().empty())
		{
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_CAINFO, "./cacert.pem");
		}else{
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_CAINFO, sess->httpsCacerFile().c_str());
		}
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_CAINFO failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		
		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_COOKIESESSION, 1);
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_COOKIESESSION failed!err:%s", curl_easy_strerror(tRetCode));
	    }

		switch(sess->httpRequestVer())
		{
			case CURLHTTP10:
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
				break;
			case CURLHTTP11:
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
				break;
			case CURLHTTP20:
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
				break;
			case CURLHTTP2TLS:
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
				break;
			case CURLHTTPNONE:
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_NONE);
				break;
			default:
				break;
		}
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_HTTP_VERSION failed!err:%s", curl_easy_strerror(tRetCode));
	    }

		switch(sess->httpRequestType())
		{
			case CURLHTTP_GET:
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTPGET, 1);
				break;
			case CURLHTTP_POST:
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_POST, 1L);
				if(!sess->httpRequestData().empty())
				{
					tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_POSTFIELDSIZE, sess->httpRequestData().length());
					if (CURLE_OK != tRetCode)
				    {
				        WARN("curl_easy_setopt CURLOPT_POSTFIELDSIZE failed!err:%s", curl_easy_strerror(tRetCode));
				    }
					
			    	tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_POSTFIELDS, sess->httpRequestData().c_str());
					if (CURLE_OK != tRetCode)
				    {
				        WARN("curl_easy_setopt CURLOPT_POSTFIELDS failed!err:%s", curl_easy_strerror(tRetCode));
				    }
				}
				break;
			case CURLHTTP_PUT:
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_CUSTOMREQUEST, "PUT");
				break;
			case CURLHTTP_DELETE:
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_CUSTOMREQUEST, "DELETE");
				break;
			case CURLHTTP_UPDATE:
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_CUSTOMREQUEST, "UPDATE");
				break;
			default:
				WARN("sess->httpRequestType() %d no support now", sess->httpRequestType());
				WARN("requetHttpServer new request url error");
				sess->setHttpResponseCode(-2);
				sess->setHttpReqErrorMsg("connInfo httpRequestType no support");
				CURL_HTTP_CLI::HttpResponseQueue::instance().httpResponse(sess);
				delete conn;
				return;
		}
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_CUSTOMREQUEST failed!err:%s", curl_easy_strerror(tRetCode));
	    }

		char headBuf[1024] = {0};
		HttpHeadPrivate headV = sess->httpReqPrivateHead();
		struct curl_slist* headers = NULL;
		for(size_t i = 0; i < headV.size(); i++)
		{
			memset(headBuf, 0, 1024);
			sprintf(headBuf,"%s", headV[i].c_str());
			//DEBUG("headers %s ", headBuf);
			headers = curl_slist_append(headers, headBuf);
		}
		if(headers)
		{
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTPHEADER, headers);
			if (CURLE_OK != tRetCode)
		    {
		        WARN("curl_easy_setopt CURLOPT_HTTPHEADER failed!err:%s", curl_easy_strerror(tRetCode));
		    }
			conn->connInfoSetHeader(headers);
		}

		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_URL, conn->connInfoReqUrl());
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_URL failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_WRITEFUNCTION, write_cb);
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_WRITEFUNCTION failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_WRITEDATA, conn);
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_WRITEDATA failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		//tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_VERBOSE, 1L);
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_VERBOSE failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_ERRORBUFFER, conn->connInfoErrorMsg());
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_ERRORBUFFER failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_PRIVATE, conn);
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_PRIVATE failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_NOPROGRESS, 0L);
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_NOPROGRESS failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_PROGRESSFUNCTION, prog_cb);
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_PROGRESSFUNCTION failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_PROGRESSDATA, conn);
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_PROGRESSDATA failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_FOLLOWLOCATION, 1L);
		if (CURLE_OK != tRetCode)
	    {
	        WARN("curl_easy_setopt CURLOPT_FOLLOWLOCATION failed!err:%s", curl_easy_strerror(tRetCode));
	    }
		
		conn->connInfoSetReqinfo(sess);
		conn->connInfoSetGlobal(gInfo_);
		connVectPtr->httpcliConnAdd(conn);
	}else{
		if(CURL_HTTP_CLI::CurlHttpCli::instance().httpIsKeepAlive() == 0)
		{
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_TIMEOUT, sess->httpReqDataoutSecond());
			if (CURLE_OK != tRetCode)
		    {
		        WARN("curl_easy_setopt CURLOPT_TIMEOUT failed!err:%s", curl_easy_strerror(tRetCode));
		    }
			
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_CONNECTTIMEOUT, sess->httpReqConnoutSecond());
		    if (CURLE_OK != tRetCode)
		    {
		        WARN("curl_easy_setopt CURLOPT_CONNECTTIMEOUT failed!err:%s", curl_easy_strerror(tRetCode));
		    }

			if(sess->httpsSslVerifyPeer())
			{
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_SSL_VERIFYPEER, 1L);
			}else{
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_SSL_VERIFYPEER, 0L);
			}
			if (CURLE_OK != tRetCode)
			{
				WARN("curl_easy_setopt CURLOPT_SSL_VERIFYPEER failed!err:%s", curl_easy_strerror(tRetCode));
			}

			if(sess->httpsSslVerifyHost())
			{
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_SSL_VERIFYHOST, 1L);
			}else{
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_SSL_VERIFYHOST, 0L);
			}
			if (CURLE_OK != tRetCode)
			{
				WARN("curl_easy_setopt CURLOPT_SSL_VERIFYHOST failed!err:%s", curl_easy_strerror(tRetCode));
			}

			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_AUTOREFERER, 1);
			if (CURLE_OK != tRetCode)
			{
				WARN("curl_easy_setopt CURLOPT_AUTOREFERER failed!err:%s", curl_easy_strerror(tRetCode));
			}

			if(sess->httpsCacerFile().empty())
			{
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_CAINFO, "./cacert.pem");
			}else{
				tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_CAINFO, sess->httpsCacerFile().c_str());
			}
			if (CURLE_OK != tRetCode)
			{
				WARN("curl_easy_setopt CURLOPT_CAINFO failed!err:%s", curl_easy_strerror(tRetCode));
			}
			
			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_COOKIESESSION, 1);
			if (CURLE_OK != tRetCode)
			{
				WARN("curl_easy_setopt CURLOPT_COOKIESESSION failed!err:%s", curl_easy_strerror(tRetCode));
			}

			switch(sess->httpRequestVer())
			{
				case CURLHTTP10:
					tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
					break;
				case CURLHTTP11:
					tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
					break;
				case CURLHTTP20:
					tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
					break;
				case CURLHTTP2TLS:
					tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
					break;
				case CURLHTTPNONE:
					tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_NONE);
					break;
				default:
					break;
			}
			if (CURLE_OK != tRetCode)
		    {
		        WARN("curl_easy_setopt CURLOPT_HTTP_VERSION failed!err:%s", curl_easy_strerror(tRetCode));
		    }
			
			switch(sess->httpRequestType())
			{
				case CURLHTTP_GET:
					tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTPGET, 1);
					break;
				case CURLHTTP_POST:
					tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_POST, 1L);
					if(!sess->httpRequestData().empty())
					{
						tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_POSTFIELDSIZE, sess->httpRequestData().length());
						if (CURLE_OK != tRetCode)
					    {
					        WARN("curl_easy_setopt CURLOPT_POSTFIELDSIZE failed!err:%s", curl_easy_strerror(tRetCode));
					    }
						
				    	tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_POSTFIELDS, sess->httpRequestData().c_str());
						if (CURLE_OK != tRetCode)
					    {
					        WARN("curl_easy_setopt CURLOPT_POSTFIELDS failed!err:%s", curl_easy_strerror(tRetCode));
					    }
					}
					break;
				case CURLHTTP_PUT:
					tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_CUSTOMREQUEST, "PUT");
					break;
				case CURLHTTP_DELETE:
					tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_CUSTOMREQUEST, "DELETE");
					break;
				case CURLHTTP_UPDATE:
					tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_CUSTOMREQUEST, "UPDATE");
					break;
				default:
					WARN("sess->httpRequestType() %d no support now", sess->httpRequestType());
					WARN("requetHttpServer new request url error");
					sess->setHttpResponseCode(-2);
					sess->setHttpReqErrorMsg("connInfo httpRequestType no support");
					CURL_HTTP_CLI::HttpResponseQueue::instance().httpResponse(sess);
					connQueuePtr->httpcliConnInsert(conn);
					return;
			}
			if (CURLE_OK != tRetCode)
		    {
		        WARN("curl_easy_setopt CURLOPT_CUSTOMREQUEST failed!err:%s", curl_easy_strerror(tRetCode));
		    }

			char headBuf[1024] = {0};
			HttpHeadPrivate headV = sess->httpReqPrivateHead();
			struct curl_slist* headers = NULL;
			for(size_t i = 0; i < headV.size(); i++)
			{
				memset(headBuf, 0, 1024);
				sprintf(headBuf,"%s", headV[i].c_str());
				//DEBUG("headers %s ", headBuf);
				headers = curl_slist_append(headers, headBuf);
			}

			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_HTTPHEADER, headers);
			if (CURLE_OK != tRetCode)
		    {
		        WARN("curl_easy_setopt CURLOPT_HTTPHEADER failed!err:%s", curl_easy_strerror(tRetCode));
		    }
			conn->connInfoSetHeader(headers);

			tRetCode = curl_easy_setopt(conn->connInfoEasy(), CURLOPT_URL, conn->connInfoReqUrl());
			if (CURLE_OK != tRetCode)
		    {
		        WARN("curl_easy_setopt CURLOPT_URL failed!err:%s", curl_easy_strerror(tRetCode));
	    	}
		}

		conn->connInfoSetReqinfo(sess);
		conn->connInfoSetGlobal(gInfo_);
	}
	
	int64_t seconds = 0;
	int64_t microSeconds = microSecondSinceEpoch(&seconds);
	conn->connInfoSetOutSecond(seconds);
	sess->setHttpReqMicroSecond(microSeconds);
	//INFO("Adding easy %p to multi %p (%s)", conn->connInfoEasy(), gInfo_->multi, sess->httpRequestUrl().c_str());
	CURLMcode rc = curl_multi_add_handle(gInfo_->multi, conn->connInfoEasy());
	mcode_or_die("new_conn: curl_multi_add_handle", rc);
}

int AsyncCurlHttp::mcode_or_die(const char *where, CURLMcode code)
{
	int ret = 0;
	//DEBUG("mcode_or_die %s %d", where, code);
	if(CURLM_OK != code)
	{
		const char *s = "Init";
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
				WARN( "ERROR: %s returns %s\n", where, s);
				/* ignore this error */
			return ret;
		}
		WARN("ERROR: %s returns %s\n", where, s);
		//exit(code);
		ret = -1;
	}
	return ret;
}

/* Check for completed transfers, and remove their easy handles */
void AsyncCurlHttp::check_multi_info()
{
	char *eff_url;
	CURLMsg *msg;
	int msgs_left;
	ConnInfo *conn = NULL;
	CURL *easy;
	CURLcode res;

	//INFO("REMAINING: %d", gInfo_->still_running);
	do{
		msg = curl_multi_info_read(gInfo_->multi, &msgs_left);
		//INFO("curl_multi_info_read: %d", msgs_left);
		if(msg && msg->msg == CURLMSG_DONE)
		{
			long response_code = 0;
			easy = msg->easy_handle;
			res = msg->data.result;
			curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
			curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
			curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response_code);
			INFO("DONE: %s => (%d) %ld %s", eff_url, res, response_code, conn->connInfoErrorMsg());

			conn->connMultiRemoveHandle();
			if(res)
			{
				conn->connInfoReqinfo()->setHttpResponseCode(static_cast<int>(res));
				conn->connInfoReqinfo()->setHttpReqErrorMsg(conn->connInfoErrorMsg());
			}else{
				conn->connInfoReqinfo()->setHttpResponseCode(static_cast<int>(response_code));
			}
			
			int64_t microSeconds = microSecondSinceEpoch();
			conn->connInfoReqinfo()->setHttpRspMicroSecond(microSeconds);

			if(!conn->connInfoRspBody().empty())
			{
				conn->connInfoReqinfo()->setHttpResponstBody(conn->connInfoRspBody());
			}
			CURL_HTTP_CLI::HttpResponseQueue::instance().httpResponse(conn->connInfoReqinfo());

			conn->connInfoReinit();
			connQueuePtr->httpcliConnInsert(conn);
		}
	}while(msg);

	requetHttpServer();
	
	if(gInfo_->still_running == 0 && gInfo_->stopped)//fix me
	{
		event_base_loopbreak(gInfo_->evbase);
	}
}

/* Initialize a new SockInfo structure */
void AsyncCurlHttp::addsock(curl_socket_t s, CURL *easy, int action, GlobalInfo *g)
{
	//SockInfo *fdp = static_cast<SockInfo *>(calloc(sizeof(SockInfo), 1));
	SockInfo *fdp = (SockInfo *)malloc(sizeof(SockInfo));
	if(fdp == NULL)
	{
		WARN("addsock new SockInfo error");
		return;
	}
	memset(fdp , 0 ,sizeof(SockInfo));
	
	fdp->global = g;
	setsock(fdp, s, easy, action, g);
	curl_multi_assign(gInfo_->multi, s, fdp);
}

/* Assign information to a SockInfo structure */
void AsyncCurlHttp::setsock(SockInfo *f, curl_socket_t s, CURL *e, int act, GlobalInfo *g)
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
void AsyncCurlHttp::remsock(SockInfo *f)
{
	//DEBUG("AsyncCurlHttp remsock %p", f->easy);
	if(f)
	{
		event_del(&f->ev);
		free(f);
	}
}

/* Called by libevent when we get action on a multi socket */
void AsyncCurlHttp::curlEventFdcb(int fd, short kind)
{
	CURLMcode rc;

	int action = (kind & EV_READ ? CURL_CSELECT_IN : 0) | (kind & EV_WRITE ? CURL_CSELECT_OUT : 0);

	rc = curl_multi_socket_action(gInfo_->multi, fd, action, &gInfo_->still_running);
	mcode_or_die("curlEventFdcb: curl_multi_socket_action", rc);

	check_multi_info();
	if(gInfo_->still_running <= 0)
	{
		//DEBUG("last transfer done, kill timeout");
		if(evtimer_pending(&gInfo_->timer_event, NULL))
		{
			evtimer_del(&gInfo_->timer_event);
		}
	}
}

void AsyncCurlHttp::culrMultiTimerCb(long timeout_ms)
{
	struct timeval timeout;
	CURLMcode rc;

	timeout.tv_sec = timeout_ms / 1000;
	timeout.tv_usec = (timeout_ms % 1000)*1000;
	//DEBUG("multi_timer_cb: Setting timeout to %ld ms", timeout_ms);

	/* TODO
	*
	* if timeout_ms is 0, call curl_multi_socket_action() at once!
	*
	* if timeout_ms is -1, just delete the timer
	*
	* for all other values of timeout_ms, this should set or *update*
	* the timer to the new value
	*/
	if(timeout_ms == 0)
	{
		rc = curl_multi_socket_action(gInfo_->multi, CURL_SOCKET_TIMEOUT, 0, &gInfo_->still_running);
		mcode_or_die("culrMultiTimerCb: curl_multi_socket_action", rc);
	}
	else if(timeout_ms == -1)
	{
		//DEBUG("culrMultiTimerCb delete time");
		evtimer_del(&gInfo_->timer_event);
	}
	else
	{
		evtimer_add(&gInfo_->timer_event, &timeout);
	}
}


}

