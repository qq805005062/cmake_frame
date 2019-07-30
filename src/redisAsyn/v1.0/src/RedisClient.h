#ifndef __REDIS_CLIENT_ASYNC_H__
#define __REDIS_CLIENT_ASYNC_H__

#include "vector"

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"

#include <event.h>

#include <async.h>

#include "Atomic.h"
#include "OrderInfo.h"
#include "../RedisAsync.h"

#define REDIS_CLIENT_STATE_INIT         (0)
#define REDIS_CLIENT_STATE_CONN         (1)

#define RESOURCES_FREE_ALREADY          (0)
#define RESOURCES_NEED_FREE             (1)

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
        PTRACE("%s::%d ~RedisSvrInfo exit", ipAddr_.c_str(), port_);
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

    bool operator ==(const RedisSvrInfo& that)
    {
        if(this == &that)
        {
            return true;
        }
        
        if((port_ == that.port_) && (ipAddr_.compare(that.ipAddr_) == 0))
        {
            return true;
        }

        return false;
    
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

    void disConnect(bool isFree = false);

    void requestCmd(const common::OrderNodePtr& order, uint64_t nowSecond);

    void requestCallBack(void* priv, redisReply* reply);

    void checkOutSecondCmd(uint64_t nowSecond);

    void setStateConnected()
    {
        PTRACE("%s::%d connect success", svrInfo_->ipAddr_.c_str(), svrInfo_->port_);
        freeTimeEvent();
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

    RedisSvrInfoPtr redisCliSvrInfo()
    {
        return svrInfo_;
    }
    
    size_t redisMgrfd()
    {
        return mgrFd_;
    }

    void setReleaseState()
    {
        releaseState_ = 1;
    }
    
    int releaseState()
    {
        return releaseState_;
    }
private:

    //清楚掉内部未回应的请求缓存，在连接被动或者主动断掉的时候，需要把请求缓存全部清掉
    void clearCmdReqBuf();

    //释放定时器，当连接成功之后，定时器未到时释放
    void freeTimeEvent()
    {
        if(timev_)
        {
            event_free(timev_);
            timev_ = nullptr;
        }
    }
    
    int state_;//0是未连接，1是已经连接
    int connOutSecond_;//连接超时时间，单位秒钟，会用定时器定时
    int keepAliveSecond_;//保持活跃时间，超过时间会发一个ping，无回调函数
    int releaseState_;//外部设置集群失效时，会将集群中所有的标志位设置为1
    volatile int freeState_;//内部资源释放标志位。因为断开连接会触发回调函数，disconnect不可以重入，0 是已经释放了，1是需要释放
    
    size_t ioIndex_;//io线程线程。当前这个连接绑定在那个io线程上，下标从0开始
    size_t mgrFd_;//redis管理组下标。当前这个连接是那个
    uint64_t lastSecond_;//最后一次与redis活跃时间

    struct event *timev_;//定时器指针。
    struct event_base *base_;//io线程传递进来的。
    redisAsyncContext *client_;//hredis的指针，一定要小心使用，不知道hredis内部怎么使用，避免core

    RedisSvrInfoPtr svrInfo_;//redis svr地址信息
    common::AtomicUInt32 cmdSeq_;//命令的seq 原子顺序
    common::SeqOrderNodeMap cmdSeqOrderMap_;//缓存的命令池
};

typedef std::shared_ptr<RedisClient> RedisClientPtr;
typedef std::vector<RedisClientPtr> VectRedisClientPtr;

}
#endif // end __REDIS_CLIENT_ASYNC_H__