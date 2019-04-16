
#include <unistd.h>

#include "Incommon.h"
#include "RedisClient.h"

namespace REDIS_ASYNC_CLIENT
{
#if 0
static void connectTimeout(evutil_socket_t fd, short event, void *arg)
{
    return;
}

static void connectCallback(const redisAsyncContext *c, int status)
{
    return;
}

static void disconnectCallback(const redisAsyncContext *c, int status)
{
    return;
}

static void cmdCallback(redisAsyncContext *c, void *r, void *privdata)
{
    return;
}
#endif

RedisClient::RedisClient(size_t ioIndex, const std::string& ipaddr, int port, int cmdOutSecond, int connOutSecond)
{
}

RedisClient::~RedisClient()
{
}

int RedisClient::connectServer(struct event_base* eBase)
{
    return 0;
}

void RedisClient::disConnect()
{
    if(timev_)
    {
        event_free(timev_);
        timev_ = nullptr;
    }
    if(client_)
    {
        redisAsyncDisconnect(client_);
        client_ = NULL;
    }
}

}

