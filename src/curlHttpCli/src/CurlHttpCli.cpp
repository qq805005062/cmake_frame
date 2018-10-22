
#include <curl/curl.h>

#include "Incommon.h"
#include "HttpRequestQueue.h"
#include "NumberAlign.h"
#include "AsyncCurlHttp.h"
#include "../CurlHttpCli.h"

#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace CURL_HTTP_CLI
{

CurlHttpCli::CurlHttpCli()
	:ioThreadNum(0)
	,isExit_(0)
	,isReady(0)
	,isShowtime(0)
	,maxPreConns_(0)
	,maxNewConns_(0)
	,readyNum()
	,exitNum()
	,lastIndex()
	,currentConn()
	,httpCliIoPoolPtr(nullptr)
	,httpCliCallPtr(nullptr)
	,httpCliOutCallPtr(nullptr)
	,httpCliLog(nullptr)
	,curlCliVect()
{
	DEBUG("HttpClient init");
}

CurlHttpCli::~CurlHttpCli()
{
	ERROR("~HttpClient exit");
}

void CurlHttpCli::curlHttpCliExit()
{
	DEBUG("curlHttpCliExit");
	isExit_ = 1;
	int64_t num = 0;
	do{
		num = AsyncQueueNum::instance().asyncQueNum();
		if(num <= 0)
		{
			break;
		}
		DEBUG("async queue num %lu", num);
		//usleep(1000);
		sleep(5);
	}while(num > 0);

	for(int i = 0; i < ioThreadNum; i++)
	{
		if(curlCliVect[i])
		{
			curlCliVect[i]->asyncCurlExit();
		}
	}
	
	while(isReady)
	{
		usleep(1000);
	}
	
	curl_global_cleanup();

	
	for(int i = 0; i < ioThreadNum; i++)
	{
		if(curlCliVect[i])
		{
			delete curlCliVect[i];
			curlCliVect[i] = NULL;
		}
	}
	DEBUG("delete curlCliVect[i]");
	if(httpCliIoPoolPtr)
	{
		httpCliIoPoolPtr->stop();
	}
	DEBUG("httpCliIoPoolPtr->stop");

	HttpAsyncQueue::instance().stopExit();
	DEBUG("HttpAsyncQueue::instance()");
	
	if(httpCliCallPtr)
	{
		httpCliCallPtr->join();
	}
	DEBUG("httpCliCallPtr->join");
	if(httpCliOutCallPtr)
	{
		httpCliOutCallPtr->join();
	}
	DEBUG("httpCliOutCallPtr->join");
	if(httpCliLog)
	{
		httpCliLog->join();
	}
	DEBUG("httpCliLog->join");
	return;
}

int CurlHttpCli::curlHttpCliInit(int threadNum, int maxQueue, int maxPreConns, int maxNewConns, int maxSpeed, int isShowTimeUse)
{
	if(maxPreConns && maxNewConns)
	{
		WARN("paramter 3 and 4 can't all be true");
		return -3;
	}
	
	httpCliIoPoolPtr.reset(new ThreadPool("httcliIo"));
	if(httpCliIoPoolPtr == nullptr)
	{
		WARN("xiaomibiz http client thread pool new error");
		return -1;
	}
	if(maxQueue)
	{
		HttpRequestQueue::instance().setMaxQueueSize(maxQueue);
	}

	CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
	char * crulVersion = curl_version();
	INFO("curlHttpCliInit curl_global_init %d %s", code, crulVersion);

	if(isShowTimeUse)
	{
		isShowtime = isShowTimeUse;
		httpCliLog.reset(new Thread(std::bind(&CURL_HTTP_CLI::CurlHttpCli::httpStatisticsSecondThread, this), "httcliSta"));
		if(httpCliLog == nullptr)
		{
			WARN("xiaomibiz http client httpCliLog thread new error");
			return -1;
		}
		httpCliLog->start();
	}

	httpCliCallPtr.reset(new Thread(std::bind(&CURL_HTTP_CLI::CurlHttpCli::httpRspCallBackThread, this), "httcliCall"));
	if(httpCliCallPtr == nullptr)
	{
		WARN("xiaomibiz http client httpCliCallPtr thread new error");
		return -1;
	}
	httpCliCallPtr->start();

	httpCliOutCallPtr.reset(new Thread(std::bind(&CURL_HTTP_CLI::CurlHttpCli::httpOutRspCallBackThread, this), "httcliOut"));
	if(httpCliOutCallPtr == nullptr)
	{
		WARN("xiaomibiz http client httpCliOutCallPtr thread new error");
		return -1;
	}
	httpCliOutCallPtr->start();
	
	ioThreadNum = threadNum;
	maxPreConns_ = maxPreConns;
	maxNewConns_ = maxNewConns;

	curlCliVect.resize(threadNum);
	httpCliIoPoolPtr->start(threadNum);
	for(int i = 0; i < threadNum; i++)
	{
		httpCliIoPoolPtr->run(std::bind(&CURL_HTTP_CLI::CurlHttpCli::httpCliIoThread, this, i));
		curlCliVect[i] = nullptr;
	}

	return 0;
}

int CurlHttpCli::curlHttpRequest(HttpReqSession& curlReq)
{
	if(isExit_)
	{
		WARN("curlHttpRequest had been exit");
		return -2;
	}
	int64_t microSeconds = microSecondSinceEpoch();

	if(isReady == 0)
	{
		sleep(1);
	}
	HttpReqSession *req = new HttpReqSession(curlReq);
	if(req)
	{
		req->setHttpReqInsertMicroSecond(microSeconds);
		ConnInfo *conn = NULL;
		conn = (ConnInfo *)malloc(sizeof(ConnInfo));
		if(conn == NULL)
		{
			WARN("curlHttpRequest new ConnInfo error");
			delete req;
			return -1;
		}

		memset(conn, 0, sizeof (ConnInfo));
		conn->error[0] = '\0';
		conn->rspData = NULL;
		conn->headers = NULL;

		conn->easy = curl_easy_init();
		if(conn->easy == NULL)
		{
			WARN("curlHttpRequest curl_easy_init() failed, exiting!");
			delete req;
			free(conn);
			return -1;
		}

		std::string url = req->httpRequestUrl();
		conn->url = static_cast<char *>(malloc((url.size() + 1)));
		if(conn->url == NULL)
		{
			WARN("curl_easy_init() failed, exiting!\n");
			delete req;
			free(conn);
			return -1;
		}
		memset(conn->url, 0, (url.size() + 1));
		memcpy(conn->url, url.c_str(), url.size());

		CURLcode tRetCode = curl_easy_setopt(conn->easy, CURLOPT_TIMEOUT, req->httpReqDataoutSecond());
		if (CURLE_OK != tRetCode)
		{
			WARN("curl_easy_setopt CURLOPT_TIMEOUT failed!err:%s", curl_easy_strerror(tRetCode));
		} 
		tRetCode = curl_easy_setopt(conn->easy, CURLOPT_CONNECTTIMEOUT, req->httpReqConnoutSecond());
		if (CURLE_OK != tRetCode)
		{
			WARN("curl_easy_setopt CURLOPT_CONNECTTIMEOUT failed!err:%s", curl_easy_strerror(tRetCode));
		}
		tRetCode = curl_easy_setopt(conn->easy, CURLOPT_NOSIGNAL, 1);
		if (CURLE_OK != tRetCode)
		{
			WARN("curl_easy_setopt CURLOPT_NOSIGNAL failed!err:%s", curl_easy_strerror(tRetCode));
		}

		curl_easy_setopt(conn->easy, CURLOPT_FORBID_REUSE, 1L);
		curl_easy_setopt(conn->easy, CURLOPT_FRESH_CONNECT, 1);
		curl_easy_setopt(conn->easy, CURLOPT_VERBOSE, 1L);

		//https
		if(req->httpRequestVer() == CURLHTTPS)
		{
			//DEBUG("curl_easy_init https request");
			if(req->httpsSslVerifyPeer())
			{
				curl_easy_setopt(conn->easy, CURLOPT_SSL_VERIFYPEER, true);
			}else{
				curl_easy_setopt(conn->easy, CURLOPT_SSL_VERIFYPEER, 0);
			}
			if(req->httpsSslVerifyHost())
			{
				curl_easy_setopt(conn->easy, CURLOPT_SSL_VERIFYHOST, true);
			}else{
				curl_easy_setopt(conn->easy, CURLOPT_SSL_VERIFYHOST, 0);
			}
			curl_easy_setopt(conn->easy, CURLOPT_FOLLOWLOCATION, 1);
			curl_easy_setopt(conn->easy, CURLOPT_AUTOREFERER, 1);
			curl_easy_setopt(conn->easy, CURLOPT_CAINFO, "./cacert.pem");
		}

		switch(req->httpRequestType())
		{
			case CURLHTTP_GET:
				curl_easy_setopt(conn->easy, CURLOPT_HTTPGET, 1);
				break;
			case CURLHTTP_POST:
				curl_easy_setopt(conn->easy, CURLOPT_POST, 1L);
				break;
			case CURLHTTP_PUT:
				curl_easy_setopt(conn->easy, CURLOPT_CUSTOMREQUEST, "PUT");
				break;
			case CURLHTTP_DELETE:
				curl_easy_setopt(conn->easy, CURLOPT_CUSTOMREQUEST, "DELETE");
				break;
			case CURLHTTP_UPDATE:
				curl_easy_setopt(conn->easy, CURLOPT_CUSTOMREQUEST, "UPDATE");
				break;
			default:
				WARN("reqInfo->curlHttpReqType %d no support now", req->httpRequestType());
				delete req;
				free(conn->url);
				free(conn);
				return -2;
		}

		std::string bodyData = req->httpRequestData();
		if(!bodyData.empty())
		{
			//DEBUG("curl_easy_init set http body");
			curl_easy_setopt(conn->easy, CURLOPT_POSTFIELDSIZE, bodyData.length());
			curl_easy_setopt(conn->easy, CURLOPT_POSTFIELDS, bodyData.c_str());
		}

		char headBuf[1024] = {0};
		HttpHeadPrivate headV = req->httpReqPrivateHead();
		for(size_t i = 0; i < headV.size(); i++)
		{
			memset(headBuf, 0, 1024);
			sprintf(headBuf,"%s", headV[i].c_str());
			//DEBUG("headers %s ", headBuf);
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

		conn->reqInfo = req;
		
		AsyncQueueNum::instance().asyncReq();
		HttpRequestQueue::instance().httpRequest(conn);
		curlHttpClientWakeup();
		return 0;
	}else{
		WARN("curlHttpRequest new HttpReqSession error");
	}
	return -1;
}

void CurlHttpCli::curlHttpClientWakeup()
{
	while(1)
	{
		int index = lastIndex.incrementAndGet();

		index = index % ioThreadNum;
		if(curlCliVect[index])
		{
			curlCliVect[index]->wakeup();
			break;
		}else{
			WARN("httpClientWakeup error index %d", index);
		}
	}
	return;
}

void CurlHttpCli::curlHttpThreadReady()
{
	int num = readyNum.incrementAndGet();
	if(num == ioThreadNum)
	{
		INFO("curlHttpThreadReady all thread ready");
		isReady = 1;
	}
}

void CurlHttpCli::curlHttpThreadExit()
{
	int num = exitNum.incrementAndGet();
	if(num == ioThreadNum)
	{
		INFO("curlHttpThreadReady all exit");
		isReady = 0;
	}
}

void CurlHttpCli::httpCliIoThread(int index)
{
	while(1)
	{
		if(curlCliVect[index])
		{
			delete curlCliVect[index];
			curlCliVect[index] = NULL;
		}
		curlCliVect[index] = new AsyncCurlHttp(isShowtime);
		if(curlCliVect[index] == nullptr)
		{
			WARN("xiaomibiz http client new curl object error");
			continue;
		}
		try{
			curlCliVect[index]->curlHttpClientReady();
			if(isExit_)
			{
				break;
			}
		}catch (const std::exception& ex)
		{
			WARN("httpCliIoThread exception :: %s", ex.what());
		}
	}
	curlHttpThreadExit();
}

void CurlHttpCli::httpStatisticsSecondThread()
{
	curlHttpThreadReady();
	while(1)
	{
		sleep(1);
		TimeUsedUp::instance().timeUsedStatistics();
		if(isExit_)
		{
			break;
		}
	}
	curlHttpThreadExit();
}

void CurlHttpCli::httpRspCallBackThread()
{
	do{
		//DEBUG("httpRspCallBackThread httpResponseQueueTake");
		ConnInfo *conn = HttpResponseQueue::instance().httpResponseQueueTake();
		if(conn)
		{
			//DEBUG("httpRspCallBackThread ");
			int64_t microSeconds = microSecondSinceEpoch();
			conn->reqInfo->setHttpRspMicroSecond(microSeconds);
			conn->reqInfo->httpRespondCallBack();
			AsyncQueueNum::instance().asyncRsp();

			if(isShowtime)
			{
				TimeUsedUp::instance().timeUsedCalculate(conn->reqInfo->httpReqInsertMicroSecond(), conn->reqInfo->httpReqMicroSecond(), microSeconds);
			}
		}
		//DEBUG("HttpAsyncQueue::instance().queueSize() %ld HttpResponseQueue::instance().queueSize() %ld",HttpAsyncQueue::instance().queueSize(), HttpResponseQueue::instance().queueSize());
		if(isExit_ && HttpAsyncQueue::instance().queueSize() == 0 && HttpResponseQueue::instance().queueSize() == 0)
		{
			break;
		}
	}while(1);
	DEBUG("httpRspCallBackThread exit");
	return;
}

void CurlHttpCli::httpOutRspCallBackThread()
{
	bool freeFlag = false;
	do{
		usleep(100);
		//DEBUG("httpOutRspCallBackThread httpAsyncQueueFirst");
		ConnInfo *conn = HttpAsyncQueue::instance().httpAsyncQueueFirst();
		if(conn)
		{
			int64_t second = secondSinceEpoch();
			freeFlag = false;
			if(conn->reqInfo->httpResponseCallFlag())
			{
				HttpAsyncQueue::instance().httpAsyncQueueErase();
				//DEBUG("httpOutRspCallBackThread httpResponseCallFlag");

				freeFlag = true;
			}else if(second > conn->reqInfo->httpRequestOutSecond())//这个超时时间比如确保肯定超时或者连接死掉了，否则很有可能会异常
			{
				HttpAsyncQueue::instance().httpAsyncQueueErase();
				//DEBUG("httpOutRspCallBackThread httpRequestOutSecond");
				curl_multi_remove_handle(conn->global->multi, conn->easy);
				curl_easy_cleanup(conn->easy);

				int64_t microSeconds = microSecondSinceEpoch();
				conn->reqInfo->setHttpRspMicroSecond(microSeconds);
				conn->reqInfo->httpRespondCallBack();
				AsyncQueueNum::instance().asyncRsp();
				if(isShowtime)
				{
					TimeUsedUp::instance().timeUsedCalculate(conn->reqInfo->httpReqInsertMicroSecond(), conn->reqInfo->httpReqMicroSecond(), microSeconds);
				}

				freeFlag = true;
			}

			if(freeFlag)
			{
				if(conn->reqInfo)
				{
					delete conn->reqInfo;
					conn->reqInfo = NULL;
				}

				if(conn->url)
				{
					free(conn->url);
					conn->url = NULL;
				}
				if(conn->rspData)
				{
					free(conn->rspData);
					conn->rspData = NULL;
				}
				if(conn->headers)
				{
					curl_slist_free_all(conn->headers);
					conn->headers = NULL;
				}

				free(conn);
				conn = NULL;
			}
		}
		//DEBUG("HttpAsyncQueue::instance().queueSize() %ld HttpResponseQueue::instance().queueSize() %ld",HttpAsyncQueue::instance().queueSize(), HttpResponseQueue::instance().queueSize());
		if(isExit_ && HttpAsyncQueue::instance().queueSize() == 0)
		{
			HttpResponseQueue::instance().stopExit();
			break;
		}
	}while(1);
	DEBUG("httpOutRspCallBackThread exit");
	return;
}


}

