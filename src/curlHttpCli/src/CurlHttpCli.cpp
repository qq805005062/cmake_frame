
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
	,isKeepAlive(0)
	,lastIndex(0)
	,ioThreadNum(0)
	,ioMaxConns(0)
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

int CurlHttpCli::curlHttpCliInit(unsigned int threadNum, unsigned int maxQueue, unsigned int isKeepAlive, unsigned int maxConns,unsigned int maxSpeed)
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

	if(maxQueue)
	{
		CURL_HTTP_CLI::HttpRequestQueue::instance().setMaxQueueSize(maxQueue);
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
	
	if(maxConns)
	{
		ioMaxConns = maxConns / threadNum;
	}

	asyncCurlHttpPtrVect.resize(threadNum);
	httpCliIoPoolPtr->start(threadNum);
	for(size_t i = 0; i < threadNum; i++)
	{
		asyncCurlHttpPtrVect[i].reset();
		httpCliIoPoolPtr->run(std::bind(&CURL_HTTP_CLI::CurlHttpCli::httpCliIoThread, this, i));
	}

	this->isKeepAlive = isKeepAlive;
	ioThreadNum = threadNum;
	return 0;
	
}

int CurlHttpCli::curlHttpRequest(HttpReqSession& curlReq)
{
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
		CURL_HTTP_CLI::HttpRequestQueue::instance().httpRequest(req);
		curlHttpClientWakeup();
		return 0;
	}
	return -1;
}

void CurlHttpCli::curlHttpClientWakeup()
{
	int index = 0;
	unsigned int subIndex = 0;
	while(1)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			subIndex = lastIndex++;
		}

		index = static_cast<int>(subIndex % ioThreadNum);

		if(asyncCurlHttpPtrVect[index])
		{
			asyncCurlHttpPtrVect[index]->wakeup();
			break;
		}else{
			WARN("curlHttpClientWakeup error index %d", index);
		}
	}
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
		asyncCurlHttpPtrVect[index].reset(new AsyncCurlHttp(ioMaxConns));
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

void HttpConnInfoVector::httpcliConnForEach(HttpConnInfoQueuePtr& connInfoQue)
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
				connInfoQue->httpcliConnInsert(vector_[i].get());
			}
		}
	}
}


}

