
#ifndef __XIAO_HTTP_REQUEST_QUEUE_H__
#define __XIAO_HTTP_REQUEST_QUEUE_H__

#include <deque>
#include <vector>

#include "noncopyable.h"
#include "Singleton.h"
#include "MutexLock.h"
#include "Condition.h"

#include "ConnInfo.h"
#include "../HttpReqSession.h"

//#include "HttpCliQueue.h"

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
		SafeMutexLock lock(mutex_);
		while (isFull())
	    {
	    	notFull_.wait();
	    }
		queue_.push_back(req);
		//queue_.push_back(std::move(req));
	}

	HttpReqSession* dealRequest()
	{
		SafeMutexLock lock(mutex_);
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
		SafeMutexLock lock(mutex_);
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

class HttpResponseQueue : public noncopyable
{
public:
	HttpResponseQueue()
		:isExit_(0)
		,mutex_()
		,notEmpty_(mutex_)
		,queue_()
	{
	}

	~HttpResponseQueue()
	{
	}

	static HttpResponseQueue& instance() { return Singleton<HttpResponseQueue>::instance();}

	void httpResponseExit()
	{
		isExit_ = 1;
		notEmpty_.notifyAll();
	}

	void httpResponse(HttpReqSession* req)
	{
		SafeMutexLock lock(mutex_);
		queue_.push_back(req);
		notEmpty_.notify();
		//queue_.push_back(std::move(req));
	}

	HttpReqSession* dealResponse()
	{
		SafeMutexLock lock(mutex_);
		HttpReqSession *req = NULL;
		while(queue_.empty())
		{
			if(isExit_)
			{
				return req;
			}
			notEmpty_.wait();
		}

		req = queue_.front();
		queue_.pop_front();

		return req;
	}

	size_t queueSize()
	{
		SafeMutexLock lock(mutex_);
  		return queue_.size();
	}
	
private:
	
	int isExit_;
	MutexLock mutex_;
	Condition notEmpty_;
	std::deque<HttpReqSession*> queue_;

};

class HttpConnInfoQueue
{
public:
	HttpConnInfoQueue()
		:mutex_()
		,queue_()
	{
	}

	~HttpConnInfoQueue()
	{
	}

	void httpcliConnInsert(ConnInfo* conn)
	{
		SafeMutexLock lock(mutex_);
		queue_.push_back(conn);
		//queue_.push_back(std::move(req));
		//HTTPCLI::HttpClient::instance().httpClientWakeup();
	}

	ConnInfo* httpcliConnPop()
	{
		SafeMutexLock lock(mutex_);
		ConnInfo* conn = NULL;
		if (!queue_.empty())
		{
			conn = queue_.front();
			queue_.pop_front();
		}
		return conn;
	}

	size_t queueSize()
	{
		SafeMutexLock lock(mutex_);
  		return queue_.size();
	}
	
private:
	
	MutexLock mutex_;
	std::deque<ConnInfo*> queue_;
};
typedef std::shared_ptr<HttpConnInfoQueue> HttpConnInfoQueuePtr;

class HttpConnInfoVector
{
public:
	HttpConnInfoVector()
		:maxSize_(0)
		,mutex_()
		,vector_()
	{
	}

	~HttpConnInfoVector()
	{
	}

	void setMaxVectorSize(size_t maxSize)
	{ maxSize_ = maxSize; }
	
	void httpcliConnAdd(ConnInfo* conn)
	{
		ConnInfoPtr connptr(conn);
		SafeMutexLock lock(mutex_);
		vector_.push_back(connptr);
	}
	
	void httpcliConnAdd(ConnInfoPtr conn)
	{
		SafeMutexLock lock(mutex_);
		vector_.push_back(conn);
	}

	void httpcliConnForEach(HttpConnInfoQueuePtr& connInfoQue);

	size_t vectorSize()
	{
		SafeMutexLock lock(mutex_);
  		return vector_.size();
	}

	bool isFull()
	{
		SafeMutexLock lock(mutex_);
		return maxSize_ > 0 && vector_.size() >= maxSize_;
	}
private:
	size_t maxSize_;
	MutexLock mutex_;
	std::vector<ConnInfoPtr> vector_;
};
typedef std::shared_ptr<HttpConnInfoVector> HttpConnInfoVectorPtr;

}

#endif
