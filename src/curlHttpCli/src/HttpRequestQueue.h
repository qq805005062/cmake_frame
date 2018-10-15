#ifndef __XIAO_HTTP_REQUEST_QUEUE_H__
#define __XIAO_HTTP_REQUEST_QUEUE_H__

#include <deque>

#include "noncopyable.h"
#include "Singleton.h"
#include "MutexLock.h"
#include "Condition.h"
#include "../HttpReqSession.h"

namespace CURL_HTTP_CLI
{
class HttpRequestQueue : public noncopyable
{
public:
	HttpRequestQueue()
		:maxSize_(0)
		,mutex_()
		,notFull_(mutex_)
		,queue_()
	{
	}

	~HttpRequestQueue()
	{
	}

	static HttpRequestQueue& instance() { return Singleton<HttpRequestQueue>::instance();}

	void setMaxQueueSize(int maxSize) { maxSize_ = maxSize; }

	void httpRequest(HttpReqSession* req)
	{
		MutexLockGuard lock(mutex_);
		while (isFull())
	    {
	    	notFull_.wait();
	    }
		queue_.push_back(req);
		//queue_.push_back(std::move(req));
	}

	HttpReqSession* dealRequest()
	{
		MutexLockGuard lock(mutex_);
		HttpReqSession *req = NULL;
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

	size_t queueSize()
	{
		MutexLockGuard lock(mutex_);
  		return queue_.size();
	}
	
private:

	bool isFull() const
	{
		return maxSize_ > 0 && queue_.size() >= maxSize_;
	}
	
	size_t maxSize_;
	MutexLock mutex_;
	Condition notFull_;
	std::deque<HttpReqSession*> queue_;
};

}
#endif