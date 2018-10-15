
#include "Incommon.h"
#include "HttpRequestQueue.h"
#include "NumberAlign.h"
#include "AsyncCurlHttp.h"
#include "../CurlHttpCli.h"

namespace CURL_HTTP_CLI
{

CurlHttpCli::CurlHttpCli()
	:threadNum_(0)
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
	int64_t num = 0;
	do{
		num = AsyncQueueNum::instance().asyncQueNum();
		if(num <= 0)
		{
			break;
		}
		INFO("async queue num %lu", num);
		usleep(1000);
	}while(num > 0);
	return;
}

int CurlHttpCli::curlHttpCliInit(int threadNum, int maxQueue)
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
	threadNum_ = threadNum;
	curlCliVect.resize(threadNum);
	httpCliPoolPtr->start(threadNum);
	for(int i = 0; i < threadNum; i++)
	{
		httpCliPoolPtr->run(std::bind(&CURL_HTTP_CLI::CurlHttpCli::httpIoThreadFun, this, i));
		curlCliVect[i] = nullptr;
	}

	return 0;
}

int CurlHttpCli::curlHttpRequest(HttpReqSession& curlReq)
{
	INFO("curlHttpRequest");
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
			curlCliVect[index]->wakeup();
			break;
		}else{
			WARN("httpClientWakeup error index %d", index);
		}
	}
	return;
}

void CurlHttpCli::httpIoThreadFun(int index)
{
	while(1)
	{
		if(curlCliVect[index])
		{
			free(curlCliVect[index]);
		}
		curlCliVect[index] = new AsyncCurlHttp();
		if(curlCliVect[index] == nullptr)
		{
			WARN("xiaomibiz http client new curl object error");
			continue;
		}
		try{
			curlCliVect[index]->curlHttpClientReady();
		}catch (const std::exception& ex)
		{
			WARN("httpIoThreadFun exception :: %s",ex.what());
		}
	}
}

}

