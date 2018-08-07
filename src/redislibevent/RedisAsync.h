
#ifndef __REDIS_ASYNC_H__
#define __REDIS_ASYNC_H__

#include <map>
#include <vector>
#include <functional>

#include <common/Atomic.h>
#include <common/MutexLock.h>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#define SHOW_DEBUG		1
#define SHOW_ERROR		1
#ifdef SHOW_DEBUG
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#ifdef SHOW_ERROR
#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PERROR(fmt, args...)
#endif

namespace ASYNCREDIS
{

typedef std::map<std::string, std::string> HashMap;
typedef HashMap::const_iterator HashMapConstIter;
typedef HashMap::iterator HashMapIter;

typedef std::vector<std::string> StrVector;
typedef StrVector::iterator StrVectorIter;

typedef std::function<void(int64_t ret, void *privdata, const std::string& err)> CmdResultCallBack;
typedef std::function<void(const std::string& rStr, void *privdata, const std::string& err)> CmdStrValueCallBack;
typedef std::function<void(const StrVector& strV, void *privdata, const std::string& err)> CmdStrVectorCallBack;

//typedef std::function<void(const char *errMsg,int bConn)> RedisOnConnCallBack;

typedef std::vector<redisAsyncContext *> ConstRedisAsyncConn;
typedef ConstRedisAsyncConn::iterator ConstRedisAsyncConnIter;

typedef common::AtomicIntegerT<uint32_t> AtomicUInt32;

class RedisRequest
{
public:
	RedisRequest()
		:seq(0)
		,priv(nullptr)
		,retCb_(nullptr)
		,strCb_(nullptr)
		,strVCb_(nullptr)
	{
	}
	
	~RedisRequest()
	{
	}

	RedisRequest(RedisRequest& that)
	{
		this->seq = that.seq;
		this->priv = that.priv;
		this->retCb_ = that.retCb_;
		this->strCb_ = that.strCb_;
		this->strVCb_ = that.strVCb_;
	}

	RedisRequest(void* opaque, const CmdResultCallBack& cb)
	{
		priv = opaque;
		retCb_ = cb;
	}

	RedisRequest(void* opaque, const CmdStrValueCallBack& cb)
	{
		priv = opaque;
		strCb_ = cb;
	}

	RedisRequest(void* opaque, const CmdStrVectorCallBack& cb)
	{
		priv = opaque;
		strVCb_ = cb;
	}

	void SerRedisRequestSeq(uint32_t s)
	{
		seq = s;
	}

	void SetRedisRequest(void *opaque, const CmdResultCallBack& cb)
	{
		priv = opaque;
		retCb_ = cb;
	}

	void SetRedisRequest(void *opaque, const CmdStrValueCallBack& cb)
	{
		priv = opaque;
		strCb_ = cb;
	}

	void SetRedisRequest(void *opaque, const CmdStrVectorCallBack& cb)
	{
		priv = opaque;
		strVCb_ = cb;
	}

	bool IsResultCallBack()
	{
		if(retCb_)
			return true;
		else
			return false;
	}

	bool IsStrValueCallBack()
	{
		if(strCb_)
			return true;
		else
			return false;
	}

	bool IsStrVectorCallBack()
	{
		if(strVCb_)
			return true;
		else
			return false;
	}
	
	void RespondCallBack(int64_t ret, const std::string& err)
	{
		if(retCb_)
			retCb_(ret,priv,err);
	}

	void RespondCallBack(const std::string &strValue, const std::string& err)
	{
		if(retCb_)
			strCb_(strValue,priv,err);
	}

	void RespondCallBack(const StrVector &strVector, const std::string& err)
	{
		if(strVCb_)
			strVCb_(strVector,priv,err);
	}

	uint32_t RedisRespondSeq()
	{
		return seq;
	}
	
private:

	uint32_t seq;
	void *priv;
	CmdResultCallBack retCb_;
	CmdStrValueCallBack strCb_;
	CmdStrVectorCallBack strVCb_;
};

class RedisAsync
{
public:
	RedisAsync(struct event_base* base = NULL);
	~RedisAsync();

	int RedisConnect(const std::string host,int port,int num);

	int ResetConnect(const std::string host,int port);

	void RedisOnConnection(const redisAsyncContext *c,int status,bool bConn);

	void RedisLoop();

	void RedisBlockClear();

	int set(const std::string& key, const std::string& value, const CmdResultCallBack& retCb, void *priv);
	
	int get(const std::string& key, CmdStrValueCallBack strCb, void *priv);
	
	int hmset(const std::string& key, const HashMap& hashMap, const CmdResultCallBack& retCb, void *priv);

private:
	common::MutexLock IterLock;
	int asyStatus;
	struct event_base *libevent;

	int conInitNum;
	volatile int conSuccNum;
	int lastIndex;

	std::string host_;
	int port_;

	ConstRedisAsyncConn conns_;
	AtomicUInt32 seqNum;
};

}
#endif
