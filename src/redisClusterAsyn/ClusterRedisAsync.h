#ifndef __CLUSTER_REDIS_ASYNC_H__
#define __CLUSTER_REDIS_ASYNC_H__

#include <vector>
#include <functional>

namespace CLUSTER_REDIS_ASYNC
{

typedef std::vector<std::string> StdVectorString;
typedef StdVectorString::iterator StdVectorStringIter;

typedef std::function<void(int ret, const std::string& errMsg)> InitErrorCallback;

typedef std::function<void(int64_t ret, void* priv, StdVectorString& resultMsg)> QueryResultCallback;

class ClusterRedisAsync
{
public:
	ClusterRedisAsync();

	~ClusterRedisAsync();

	int redisAsyncInit(int threadNum, int connNum, const std::string& ipaddr, int port);

	int set(const std::string& key, const std::string& value, const QueryResultCallback& cb, void *priv);
	
private:

	int redisAsyncCmd(const QueryResultCallback& cb, void* priv, const char *format, ...);
	
	InitErrorCallback initCb_;
};

}
#endif