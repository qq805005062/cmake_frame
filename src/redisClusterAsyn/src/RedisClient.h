#ifndef __REDIS_CLIENT_ASYNC_H__
#define __REDIS_CLIENT_ASYNC_H__

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"

#include <event.h>

#include <async.h>
#include <hiredis.h>

#define REDIS_CLIENT_STATE_INIT         0
#define REDIS_CLIENT_STATE_CONN         1

namespace REDIS_ASYNC_CLIENT
{

class RedisClient
{
public:
    RedisClient(size_t ioIndex, const std::string& ipaddr, int port, int cmdOutSecond = 30, int connOutSecond = 3);

    ~RedisClient();

    int connectServer(struct event_base* eBase);

    void disConnect();

    int requestCmd(const std::string& cmd, void* priv = NULL);

    int tcpCliState()
    {
        return state_;
    }

    void setStateConnected()
    {
        state_ = REDIS_CLIENT_STATE_CONN;
    }
    
private:
    int state_;//0是初始化状态，1是已经连接，2是连接失败
    int port_;
    int connOutSecond_;
    int dataOutSecond_;

    size_t ioIndex_;
    
    struct event *timev_;
    struct event_base *base_;
    redisAsyncContext *client_;

    std::string ipaddr_;
};

typedef std::shared_ptr<RedisClient> RedisClientPtr;
}
#endif // end __REDIS_CLIENT_ASYNC_H__