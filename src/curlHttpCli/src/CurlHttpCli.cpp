
#include "Incommon.h"
#include "HttpRequestQueue.h"
#include "NumberAlign.h"
#include "AsyncCurlHttp.h"
#include "../CurlHttpCli.h"

namespace CURL_HTTP_CLI
{

CurlHttpCli::CurlHttpCli()
	:threadNum_(0)
	,isExit_(0)
	,isReady(0)
	,isShowtime(0)
	,readyNum()
	,exitNum()
	,lastIndex()
	,httpCliPoolPtr(nullptr)
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
	isExit_ = 1;
	int64_t num = 0;
	do{
		num = AsyncQueueNum::instance().asyncQueNum();
		if(num <= 0)
		{
			break;
		}
		//INFO("async queue num %lu", num);
		usleep(1000);
		//sleep(5);
	}while(num > 0);

	for(int i = 0; i < threadNum_; i++)
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
	httpCliPoolPtr->stop();
	return;
}

int CurlHttpCli::curlHttpCliInit(int threadNum, int maxQueue, int isShowTimeUse)
{
	INFO("httpClientInit");
	httpCliPoolPtr.reset(new ThreadPool("httcli"));
	if(httpCliPoolPtr == nullptr)
	{
		WARN("xiaomibiz http client thread pool new error");
		return -1;
	}
	if(maxQueue)
	{
		HttpRequestQueue::instance().setMaxQueueSize(maxQueue);
	}
	
	curlCliVect.resize(threadNum);
	if(isShowTimeUse)
	{
		isShowtime = isShowTimeUse;
		threadNum++;
		threadNum_ = threadNum;
		httpCliPoolPtr->start(threadNum);
		threadNum--;
		httpCliPoolPtr->run(std::bind(&CURL_HTTP_CLI::CurlHttpCli::httpStatisticsSecond, this));
	}else{
		threadNum_ = threadNum;
		httpCliPoolPtr->start(threadNum);
	}
	for(int i = 0; i < threadNum; i++)
	{
		httpCliPoolPtr->run(std::bind(&CURL_HTTP_CLI::CurlHttpCli::httpIoThreadFun, this, i));
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
	
	if(isReady == 0)
	{
		sleep(1);
	}
	//INFO("curlHttpRequest");
	HttpReqSession *req = new HttpReqSession(curlReq);
	if(req)
	{
		AsyncQueueNum::instance().asyncReq();
		HttpRequestQueue::instance().httpRequest(req);
		curlHttpClientWakeup();
		return 0;
	}
	return -1;
}

void CurlHttpCli::curlHttpClientWakeup()
{
	while(1)
	{
		int index = lastIndex.incrementAndGet();

		index = index % threadNum_;
		if(curlCliVect[index])
		{
			//INFO("curlHttpClientWakeup");
			curlCliVect[index]->wakeup();
			//curlCliVect[index]->wakeup();
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
	if(num == threadNum_)
	{
		INFO("curlHttpThreadReady all thread ready");
		isReady = 1;
	}
}

void CurlHttpCli::curlHttpThreadExit()
{
	int num = exitNum.incrementAndGet();
	if(num == threadNum_)
	{
		INFO("curlHttpThreadReady all thread ready");
		isReady = 0;
	}
}

void CurlHttpCli::httpIoThreadFun(int index)
{
	while(1)
	{
		if(curlCliVect[index])
		{
			free(curlCliVect[index]);
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
			WARN("httpIoThreadFun exception :: %s",ex.what());
		}
	}
	curlHttpThreadExit();
}

void CurlHttpCli::httpStatisticsSecond()
{
	curlHttpThreadReady();
	while(1)
	{
		sleep(1);
		if(isExit_)
		{
			break;
		}
		TimeUsedUp::instance().timeUsedStatistics();
	}
	curlHttpThreadExit();
}

}

