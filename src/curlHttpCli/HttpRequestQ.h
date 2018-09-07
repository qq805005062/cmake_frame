#ifndef __CURL_HTTP_REQUEST_Q_H__
#define __CURL_HTTP_REQUEST_Q_H__

#include <deque>

#include "noncopyable.h"
#include "Singleton.h"

#include "HttpSession.h"
#include "MutexLock.h"
#include "Condition.h"
//#include "HttpClient.h"

namespace HTTPCLI
{

class HttpRequestQ : public common::noncopyable
{
public:
	HttpRequestQ()
		:maxSize_(0)
		,mutex_()
		,notFull_(mutex_)
		,queue_()
	{
	}

	~HttpRequestQ()
	{
	}

	static HttpRequestQ& instance() { return common::Singleton<HttpRequestQ>::instance();}

	void setMaxQueueSize(int maxSize) { maxSize_ = maxSize; }

	void httpRequest(const CurlHttpRequestPtr& req)
	{
		XIAOMI::MutexLockGuard lock(mutex_);
		while (isFull())
	    {
	    	notFull_.wait();
	    }
		
		queue_.push_back(req);
		//HTTPCLI::HttpClient::instance().httpClientWakeup();
	}

	CurlHttpRequestPtr dealRequest()
	{
		XIAOMI::MutexLockGuard lock(mutex_);
		CurlHttpRequestPtr req(nullptr);
		if (!queue_.empty())
		{
			req = queue_.front();
			queue_.pop_front();
			if (maxSize_ > 0)
			{
				notFull_.notify();
			}
		}
		return req;
	}
	
private:

	bool isFull() const
	{
		return maxSize_ > 0 && queue_.size() >= maxSize_;
	}

	bool isEmpty() const
	{
		return queue_.size()?false:true;
	}
	
	size_t maxSize_;
	XIAOMI::MutexLock mutex_;
	XIAOMI::Condition notFull_;
	std::deque<CurlHttpRequestPtr> queue_;
};

}
#endif// __CURL_HTTP_REQUEST_Q_H__

