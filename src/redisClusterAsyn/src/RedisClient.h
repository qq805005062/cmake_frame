#ifndef __REDIS_CLIENT_ASYNC_H__
#define __REDIS_CLIENT_ASYNC_H__

#include "vector"

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"

#include <event.h>

#include <async.h>
#include <hiredis.h>

#include "Atomic.h"
#include "OrderInfo.h"
#include "../ClusterRedisAsync.h"

#define REDIS_CLIENT_STATE_INIT         0
#define REDIS_CLIENT_STATE_CONN         1

namespace REDIS_ASYNC_CLIENT
{

class RedisSvrInfo
{
public:
    RedisSvrInfo(int p, const std::string& ip)
        :port_(p)
        ,ipAddr_(ip)
    {
    }

    RedisSvrInfo(const std::string& ip, int p)
        :port_(p)
        ,ipAddr_(ip)
    {
    }

    ~RedisSvrInfo()
    {
    }

    RedisSvrInfo(const RedisSvrInfo& that)
        :port_(0)
        ,ipAddr_()
    {
        *this = that;
    }

    RedisSvrInfo& operator=(const RedisSvrInfo& that)
    {
        if(this == &that) return *this;

        port_ = that.port_;
        ipAddr_ = that.ipAddr_;
        
        return *this;
    }

    int port_;
    std::string ipAddr_;
};
typedef std::shared_ptr<RedisSvrInfo> RedisSvrInfoPtr;
typedef std::vector<RedisSvrInfoPtr> VectRedisSvrInfoPtr;

class RedisClient
{
public:
    RedisClient(size_t ioIndex, size_t fd, const RedisSvrInfoPtr& svrInfo, int keepAliveSecond = 10, int connOutSecond = 3);

    ~RedisClient();

    int connectServer(struct event_base* eBase);

    void disConnect();

    void requestCmd(const common::OrderNodePtr& order);

    void requestCallBack(void* priv, redisReply* reply);

    void checkOutSecondCmd(uint64_t nowSecond = 0);

    void setStateConnected()
    {
        if(timev_)
        {
            event_free(timev_);
            timev_ = nullptr;
        }
        state_ = REDIS_CLIENT_STATE_CONN;
        lastSecond_ = secondSinceEpoch();
    }

    int tcpCliState()
    {
        return state_;
    }
    
    std::string redisSvrIpaddr()
    {
        return svrInfo_->ipAddr_;
    }

    int redisSvrPort()
    {
        return svrInfo_->port_;
    }

    uint64_t lastAliveSecond()
    {
        return lastSecond_;
    }

    size_t redisCliIoIndex()
    {
        return ioIndex_;
    }
private:
    int state_;//0是初始化状态，1是已经连接，2是连接失败
    int connOutSecond_;
    int keepAliveSecond_;
    
    size_t ioIndex_;
    size_t mgrFd_;
    uint64_t lastSecond_;//最后一次与redis活跃时间

    struct event *timev_;
    struct event_base *base_;
    redisAsyncContext *client_;

    RedisSvrInfoPtr svrInfo_;
    common::AtomicUInt32 cmdSeq_;
    common::SeqOrderNodeMap cmdSeqOrderMap_;
};

typedef std::shared_ptr<RedisClient> RedisClientPtr;
typedef std::vector<RedisClientPtr> VectRedisClientPtr;

}
#endif // end __REDIS_CLIENT_ASYNC_H__