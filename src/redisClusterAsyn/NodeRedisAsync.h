#ifndef __NODE_REDIS_ASYNC_H__
#define __NODE_REDIS_ASYNC_H__
#include <vector>
#include <functional>

namespace NODEREDISASYNC
{

typedef std::vector<std::string> StdVectorString;
typedef StdVectorString::iterator StdVectorStringIter;

typedef std::function<void(int ret, const std::string& errMsg)> InitErrorCallback;

typedef std::function<void(int64_t ret, void* priv, StdVectorString& resultMsg)> QueryResultCallback;

class NodeRedisAsync
{
public:
	NodeRedisAsync();

	~NodeRedisAsync();

	void setRedisInitErrorCb(const InitErrorCallback& cb)
	{
		initCb_ = cb;
	}

	int redisAsyncInit(int threadNum, int connNum, const std::string& ipaddr, int port);

	int redisAsyncCmd(const QueryResultCallback& cb, void* priv, const char *format, ...);

private:
	
	InitErrorCallback initCb_;
};

}
#endif