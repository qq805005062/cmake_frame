#ifndef __XIAO_HTTP_REQUEST_QUEUE_H__
#define __XIAO_HTTP_REQUEST_QUEUE_H__

#include <deque>

#include "noncopyable.h"
#include "Singleton.h"
#include "MutexLock.h"
#include "Condition.h"
#include "AsyncCurlHttp.h"


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

	void httpRequest(ConnInfo* conn)
	{
		MutexLockGuard lock(mutex_);
		while (isFull())
	    {
	    	notFull_.wait();
	    }
		queue_.push_back(conn);
		//queue_.push_back(std::move(req));
	}

	ConnInfo* dealRequest()
	{
		MutexLockGuard lock(mutex_);
		ConnInfo *conn = NULL;
		if (!queue_.empty())
		{
			conn = queue_.front();
			queue_.pop_front();
			if (maxSize_ > 0)
			{
				notFull_.notify();
			}
		}
		return conn;
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
	std::deque<ConnInfo*> queue_;
};

class HttpAsyncQueue : public noncopyable
{
public:
	HttpAsyncQueue()
		:mutex_()
		,notEmpty_(mutex_)
		,queue_()
	{
	}

	~HttpAsyncQueue()
	{
	}

	static HttpAsyncQueue& instance() {return Singleton<HttpAsyncQueue>::instance();}

	void stopExit()
	{
		MutexLockGuard lock(mutex_);
		notEmpty_.notifyAll();
	}

	void httpAsyncInsert(ConnInfo* conn)
	{
		MutexLockGuard lock(mutex_);
		queue_.push_back(conn);
		//queue_.push_back(std::move(req));
		notEmpty_.notify();
	}

	ConnInfo* httpAsyncQueueFirst()
	{
		MutexLockGuard lock(mutex_);
		if (queue_.empty())
		{
			notEmpty_.wait();
		}
		
		ConnInfo *conn = NULL;
		if (!queue_.empty())
		{
			conn = queue_.front();
		}
		return conn;
	}

	void httpAsyncQueueErase()
	{
		MutexLockGuard lock(mutex_);
		queue_.pop_front();
	}
	
	size_t queueSize()
	{
		MutexLockGuard lock(mutex_);
  		return queue_.size();
	}
	
private:
	MutexLock mutex_;
	Condition notEmpty_;
	std::deque<ConnInfo*> queue_;
};


class HttpResponseQueue : public noncopyable
{
public:
	HttpResponseQueue()
		:mutex_()
		,notEmpty_(mutex_)
		,queue_()
	{
	}

	~HttpResponseQueue()
	{
	}

	static HttpResponseQueue& instance() {return Singleton<HttpResponseQueue>::instance();}

	void stopExit()
	{
		MutexLockGuard lock(mutex_);
		notEmpty_.notifyAll();
	}
	
	void httpResponseQueue(ConnInfo* conn)
	{
		DEBUG("httpResponseQueue ");
		MutexLockGuard lock(mutex_);
		queue_.push_back(conn);
		//queue_.push_back(std::move(req));
		notEmpty_.notify();
	}

	ConnInfo* httpResponseQueueTake()
	{
		MutexLockGuard lock(mutex_);
		if (queue_.empty())
		{
			notEmpty_.wait();
		}
		
		ConnInfo *conn = NULL;
		if (!queue_.empty())
		{
			DEBUG("httpResponseQueueTake ");
			conn = queue_.front();
			queue_.pop_front();
		}
		return conn;
	}
	
	size_t queueSize()
	{
		MutexLockGuard lock(mutex_);
  		return queue_.size();
	}
	
private:

	MutexLock mutex_;
	Condition notEmpty_;
	std::deque<ConnInfo*> queue_;
};

}
#endif