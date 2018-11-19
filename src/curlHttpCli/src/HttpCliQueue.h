
#ifndef __XIAO_HTTP_REQUEST_QUEUE_H__
#define __XIAO_HTTP_REQUEST_QUEUE_H__

#include <deque>
#include <vector>
#include <map>

#include "noncopyable.h"
#include "Singleton.h"
#include "MutexLock.h"
#include "Condition.h"

#include "ConnInfo.h"
#include "../HttpReqSession.h"

//#include "HttpCliQueue.h"

namespace CURL_HTTP_CLI
{

class HttpRequestQueue
{
public:
	HttpRequestQueue(size_t maxSize)
		:maxSize_(maxSize)
		,mutex_()
		,notFull_(mutex_)
		,queue_()
	{
	}

	~HttpRequestQueue()
	{
	}

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
typedef std::shared_ptr<HttpRequestQueue> HttpRequestQueuePtr;

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
	HttpConnInfoQueue(size_t maxSize = 0)
		:maxSize_(maxSize)
		,mutex_()
		,notEmpty_(mutex_)
		,queue_()
	{
	}

	~HttpConnInfoQueue()
	{
	}

	void setMaxQueueSize(int maxSize) { maxSize_ = maxSize; }
	
	void httpcliConnInsert(ConnInfo* conn, bool isNew)
	{
		SafeMutexLock lock(mutex_);
		queue_.push_back(conn);
		if(isNew)
		{
			queueSize_++;
		}
		//queue_.push_back(std::move(req));
		//HTTPCLI::HttpClient::instance().httpClientWakeup();
		notEmpty_.notify();
	}

	ConnInfo* httpcliConnPop()
	{
		SafeMutexLock lock(mutex_);
		ConnInfo* conn = NULL;
		while(queue_.empty())
		{
			if(isFull())
			{
				notEmpty_.wait();
			}else{
				return conn;
			}
		}
	
		conn = queue_.front();
		queue_.pop_front();
	
		return conn;
	}

	size_t queueSize()
	{
		SafeMutexLock lock(mutex_);
  		return queue_.size();
	}
	
private:

	bool isFull() const
	{
		return maxSize_ > 0 && queueSize_ >= maxSize_;
	}
	
	size_t maxSize_;
	size_t queueSize_;
	
	MutexLock mutex_;
	
	Condition notEmpty_;
	std::deque<ConnInfo*> queue_;
};
typedef std::shared_ptr<HttpConnInfoQueue> HttpConnInfoQueuePtr;

typedef std::map<std::string, HttpConnInfoQueuePtr> HttpUrlConnInfoMap;
typedef HttpUrlConnInfoMap::iterator HttpUrlConnInfoMapIter;


class HttpUrlConnInfo : public noncopyable
{
public:
	HttpUrlConnInfo()
		:mutex_()
		,urlMap_()
	{
	
}

	~HttpUrlConnInfo()
	{
	}

	static HttpUrlConnInfo& instance() { return Singleton<HttpUrlConnInfo>::instance();}
	
	ConnInfo* connInfoFromUrl(const std::string& url)
	{
		ConnInfo* conn = NULL;
		HttpConnInfoQueuePtr connQueue(nullptr);
		
		{
			SafeMutexLock lock(mutex_);
			HttpUrlConnInfoMapIter iter = urlMap_.find(url);
			if(iter == urlMap_.end())
			{
				return conn;
			}else{
				connQueue = iter->second;
			}
		}

		if(connQueue)
		{
			conn = connQueue->httpcliConnPop();
		}
		return conn;
		
	}

	int addUrlConnInfo(const std::string& url, size_t maxSize)
	{
		{
			SafeMutexLock lock(mutex_);
			HttpUrlConnInfoMapIter iter = urlMap_.find(url);
			if(iter != urlMap_.end())
			{
				return 1;
			}
		}

		HttpConnInfoQueuePtr connQueue(new HttpConnInfoQueue(maxSize));
		if(connQueue)
		{
			urlMap_.insert(HttpUrlConnInfoMap::value_type(url, connQueue));
		}else{
			return -1;
		}
		
		return 0;
	}

	int returnUrlConnInfo(const std::string& url, ConnInfo* conn, bool isNew)
	{
		HttpConnInfoQueuePtr connQueue(nullptr);
		
		{
			SafeMutexLock lock(mutex_);
			HttpUrlConnInfoMapIter iter = urlMap_.find(url);
			if(iter == urlMap_.end())
			{
				connQueue.reset(new HttpConnInfoQueue());
				if(connQueue)
				{
					urlMap_.insert(HttpUrlConnInfoMap::value_type(url, connQueue));
				}else{
					return -1;
				}
			}else{
				connQueue = iter->second;
			}
		}

		if(connQueue)
		{
			connQueue->httpcliConnInsert(conn, isNew);
		}else{
			return -1;
		}
		return 0;
	}
	
private:
	
	MutexLock mutex_;
	HttpUrlConnInfoMap urlMap_;
};

class HttpConnInfoVector
{
public:
	HttpConnInfoVector()
		:mutex_()
		,vector_()
	{
	}

	~HttpConnInfoVector()
	{
	}
	
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

	void httpcliConnForEach();

	size_t vectorSize()
	{
		SafeMutexLock lock(mutex_);
  		return vector_.size();
	}

private:

	MutexLock mutex_;
	std::vector<ConnInfoPtr> vector_;
};
typedef std::shared_ptr<HttpConnInfoVector> HttpConnInfoVectorPtr;

}

#endif
