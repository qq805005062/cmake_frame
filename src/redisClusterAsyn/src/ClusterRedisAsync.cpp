
#include "ClusterRedisAsync.h"

namespace CLUSTER_REDIS_ASYNC
{

ClusterRedisAsync::ClusterRedisAsync()
{
}

ClusterRedisAsync::~ClusterRedisAsync()
{
}

int ClusterRedisAsync::redisAsyncCmd(const QueryResultCallback& cb, void* priv, const char *format, ...)
{
	return 0;
}

int ClusterRedisAsync::redisAsyncInit(int threadNum, int connNum, const std::string& ipaddr, int port)
{
	return 0;
}

}

