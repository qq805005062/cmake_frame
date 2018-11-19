
#include "Incommon.h"
#include "ThreadPool.h"
#include "NumberAlign.h"
#include "ConnInfo.h"
#include "HttpCliQueue.h"
#include "AsyncCurlHttp.h"
#include "../CurlHttpCli.h"

namespace CURL_HTTP_CLI
{

static std::unique_ptr<ThreadPool> httpCliIoPoolPtr;
static std::unique_ptr<Thread> httpCliCallPtr;
static std::unique_ptr<Thread> httpIoWakePtr;
static std::vector<AsyncCurlHttpPtr> asyncCurlHttpPtrVect;

CurlHttpCli::CurlHttpCli()
	:isExit(0)
	,threadExit(0)
	,readyIothread(0)
	,exitIothread(0)
	,lastIndex(0)
	,ioThreadNum(0)
	,maxQueue_(0)
	,mutex_()
{
	DEBUG("HttpClient init");
}

CurlHttpCli::~CurlHttpCli()
{
	ERROR("~HttpClient exit");
}

void CurlHttpCli::curlHttpCliExit()
{
	isExit = 1;
	DEBUG("curlHttpCliExit");

	while(ioThreadNum == 0)
	{
		usleep(1000);
	}
	
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

	threadExit = 1;
	if(httpIoWakePtr)
	{
		httpIoWakePtr->join();
		httpIoWakePtr.reset();
	}

	for(size_t i = 0; i < ioThreadNum; i++)
	{
		if(asyncCurlHttpPtrVect[i])
		{
			asyncCurlHttpPtrVect[i]->asyncCurlExit();
		}
	}

	CURL_HTTP_CLI::HttpResponseQueue::instance().httpResponseExit();
	if(httpCliCallPtr)
	{
		httpCliCallPtr->join();
		httpCliCallPtr.reset();
	}

	while(exitIothread != ioThreadNum)
	{
		usleep(1000);
	}

	for(size_t i = 0; i < ioThreadNum; i++)
	{
		if(asyncCurlHttpPtrVect[i])
		{
			asyncCurlHttpPtrVect[i].reset();
		}
	}

	if(httpCliIoPoolPtr)
	{
		httpCliIoPoolPtr->stop();
		httpCliIoPoolPtr.reset();
	}
	
	curl_global_cleanup();
	return;
}

int CurlHttpCli::curlHttpCliInit(unsigned int threadNum, unsigned int maxQueue, unsigned int maxSpeed)
{
	CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
	char* crulVersion = curl_version();
	INFO("curlHttpCliInit curl_global_init %d %s", code, crulVersion);

	if(ioThreadNum > 0)
	{
		INFO("curlHttpCliInit had been init already");
		return 0;
	}
	httpCliIoPoolPtr.reset(new ThreadPool("httcliIo"));
	if(httpCliIoPoolPtr == nullptr)
	{
		WARN("curlHttpCliInit thread pool new error");
		return -1;
	}

	httpCliCallPtr.reset(new Thread(std::bind(&CURL_HTTP_CLI::CurlHttpCli::httpRspCallBackThread, this), "httcliCall"));
	if(httpCliCallPtr == nullptr)
	{
		WARN("CurlHttpCli http client curlHttpCliInit thread new error");
		return -1;
	}
	httpCliCallPtr->start();

	httpIoWakePtr.reset(new Thread(std::bind(&CURL_HTTP_CLI::CurlHttpCli::httpIoWakeThread, this), "httioWake"));
	if(httpIoWakePtr == nullptr)
	{
		WARN("CurlHttpCli http client httpIoWakePtr thread new error");
		return -1;
	}
	httpIoWakePtr->start();

	maxQueue_ = maxQueue;
	asyncCurlHttpPtrVect.resize(threadNum);
	httpCliIoPoolPtr->start(threadNum);
	for(size_t i = 0; i < threadNum; i++)
	{
		asyncCurlHttpPtrVect[i].reset();
		httpCliIoPoolPtr->run(std::bind(&CURL_HTTP_CLI::CurlHttpCli::httpCliIoThread, this, i));
	}

	ioThreadNum = threadNum;
	return 0;
	
}

int CurlHttpCli::curlHttpRequest(HttpReqSession& curlReq)
{
	int ret = 0;
	if(isExit)
	{
		WARN("curlHttpRequest had been exit");
		return -2;
	}

	int64_t microSeconds = microSecondSinceEpoch();
	while(ioThreadNum == 0 || readyIothread != ioThreadNum)
	{
		sleep(1);
	}

	HttpReqSession *req = new HttpReqSession(curlReq);
	if(req)
	{
		req->setHttpReqInsertMicroSecond(microSeconds);
		AsyncQueueNum::instance().asyncReq();
		ConnInfo *pConninf = CURL_HTTP_CLI::HttpUrlConnInfo::instance().connInfoFromUrl(req->httpRequestUrl());
		if(pConninf)
		{
			ret = curlHttpClientWakeup(pConninf->ioThreadSubindex(), req);
		}else{
			ret = CURL_HTTP_CLI::HttpUrlConnInfo::instance().addUrlConnInfo(req->httpRequestUrl(), req->httpUrlmaxConns());
			if(ret >= 0)
			{
				ret = curlHttpClientWakeup(ioThreadNum, req);
			}
		}
	}
	if(ret < 0)
	{
		delete req;
	}
	return ret;
}

int CurlHttpCli::curlHttpClientWakeup(size_t ioIndex, HttpReqSession *req)
{
	int ret = 0;
	size_t index = 0;
	unsigned int subIndex = 0;

	if(ioIndex == ioThreadNum)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			subIndex = lastIndex++;
		}

		index = static_cast<int>(subIndex % ioThreadNum);
	}else{
		index = ioIndex;
	}

	if(asyncCurlHttpPtrVect[index])
	{
		ret = asyncCurlHttpPtrVect[index]->curlhttpRequest(req);
		asyncCurlHttpPtrVect[index]->wakeup();
	}else{
		ret = -1;
		WARN("curlHttpClientWakeup error index %ld", index);
	}
	return ret;
}

void CurlHttpCli::curlHttpThreadReady()
{
	std::lock_guard<std::mutex> lock(mutex_);
	readyIothread++;
}

void CurlHttpCli::httpCliIoThread(size_t index)
{
	int ret = 0;
	while(1)
	{
		asyncCurlHttpPtrVect[index].reset(new AsyncCurlHttp(index, maxQueue_));
		if(asyncCurlHttpPtrVect[index] == nullptr)
		{
			WARN("CurlHttpCli http client new httpCliIoThread error");
			continue;
		}
		asyncCurlHttpPtrVect[index]->asyncCurlReady();
		{
			std::lock_guard<std::mutex> lock(mutex_);
			readyIothread--;
		}
		INFO("asyncCurlHttpPtrVect[index]->asyncCurlReady ret %d", ret);
		if(isExit)
		{
			break;
		}
	}
	{
		std::lock_guard<std::mutex> lock(mutex_);
		exitIothread++;
	}
}

void CurlHttpCli::httpRspCallBackThread()
{
	while(1)
	{
		HttpReqSession *req = CURL_HTTP_CLI::HttpResponseQueue::instance().dealResponse();
		if(req)
		{
			AsyncQueueNum::instance().asyncRsp();
			req->httpRespondCallBack();
		}
		if(threadExit)
		{
			break;
		}
	}
}

void CurlHttpCli::httpIoWakeThread()
{
	while(1)
	{
		sleep(1);
		for(size_t i = 0; i < ioThreadNum; i++)
		{
			if(asyncCurlHttpPtrVect[i])
			{
				asyncCurlHttpPtrVect[i]->wakeup();
			}
		}
		if(threadExit)
		{
			break;
		}
	}
}

void HttpConnInfoVector::httpcliConnForEach()
{
	int64_t seconds = 0;
	microSecondSinceEpoch(&seconds);
	//SafeMutexLock lock(mutex_);
	size_t allsize = vectorSize();
	for(size_t i = 0; i < allsize; i++)
	{
		if(vector_[i] && vector_[i]->connInfoOutSecond())
		{
			if((seconds - vector_[i]->connInfoOutSecond()) > ABSOLUTELY_OUT_HTTP_SECOND)
			{
				vector_[i]->connMultiRemoveHandle();
				CURL_HTTP_CLI::HttpResponseQueue::instance().httpResponse(vector_[i]->connInfoReqinfo());
				vector_[i]->connInfoReinit();
				HttpUrlConnInfo::instance().returnUrlConnInfo(vector_[i]->connInfoReqUrl(), vector_[i].get(), vector_[i]->connInfoIsNew());
			}
		}
	}
}


}

