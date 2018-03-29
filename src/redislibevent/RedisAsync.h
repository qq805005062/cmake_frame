
#ifndef __REDIS_ASYNC_H__
#define __REDIS_ASYNC_H__

#include <map>
#include <vector>
#include <functional>

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

typedef std::function<void(std::string& rStr,void *privdata)> CmdStrValueCallBack;
typedef std::function<void(int ret,void *privdata)> CmdResultCallBack;
typedef std::function<void(const char *errMsg,int bConn)> RedisOnConnCallBack;

typedef std::vector<const redisAsyncContext *> ConstRedisAsyncConn;
typedef ConstRedisAsyncConn::iterator ConstRedisAsyncConnIter;

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

	int set(const std::string& key, const std::string& value, const CmdResultCallBack& retCb);
	
	int get(const std::string& key, CmdStrValueCallBack strCb);
	
	int hmset(const std::string& key, const HashMap& hashMap, const CmdResultCallBack& retCb);

	
private:
	common::MutexLock IterLock;
	int status;
	struct event_base *libevent;

	int conInitNum;
	int conSuccNum;
	int lastIndex;

	std::string host_;
	int port_;

	ConstRedisAsyncConn conns_;
};

}
#endif
