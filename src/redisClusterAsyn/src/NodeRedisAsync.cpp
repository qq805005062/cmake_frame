
#include "NodeRedisAsync.h"

namespace NODEREDISASYNC
{

NodeRedisAsync::NodeRedisAsync()
{
}

NodeRedisAsync::~NodeRedisAsync()
{
}

int NodeRedisAsync::redisAsyncCmd(const QueryResultCallback& cb, void* priv, const char *format, ...)
{
}

int NodeRedisAsync::redisAsyncInit(int threadNum, int connNum, const std::string& ipaddr, int port)
{
	return 0;
}

}

