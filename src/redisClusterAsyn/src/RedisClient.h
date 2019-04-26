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
typedef std::vector<RedisSvrInfoPtr> RedisSvrInfoPtrVect;

class RedisClient
{
public:
    RedisClient(size_t ioIndex, const RedisSvrInfoPtr& svrInfo, int keepAliveSecond = 10, int connOutSecond = 3);

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

class RedisSvrCli
{
public:
    RedisSvrCli()
        :svrInfo_(nullptr)
        ,redisClient_(nullptr)
    {
    }

    RedisSvrCli(const RedisSvrInfoPtr& infoPtr)
        :svrInfo_(infoPtr)
        ,redisClient_(nullptr)
    {
    }

    RedisSvrCli(const RedisSvrInfoPtr& infoPtr, const RedisClientPtr& client)
        :svrInfo_(infoPtr)
        ,redisClient_(client)
    {
    }

    ~RedisSvrCli()
    {
        if(svrInfo_)
        {
            svrInfo_.reset();
        }
        if(redisClient_)
        {
            redisClient_.reset();
        }
    }

    RedisSvrCli(const RedisSvrInfo& that)
        :svrInfo_(nullptr)
        ,redisClient_(nullptr)
    {
        *this = that;
    }

    RedisSvrCli& operator=(const RedisSvrCli& that)
    {
        if(this == &that) return *this;

        svrInfo_ = that.svrInfo_;
        redisClient_ = that.redisClient_;
        
        return *this;
    }

    RedisSvrInfoPtr svrInfo_;
    RedisClientPtr redisClient_;
};
typedef std::shared_ptr<RedisSvrCli> RedisSvrCliPtr;
typedef std::vector<RedisSvrCliPtr> RedisSvrCliPtrVect;

class RedisMasterSlaveNode
{
public:
    RedisMasterSlaveNode()
        :master_(nullptr)
        ,slave_()
    {
    }

    ~RedisMasterSlaveNode()
    {
        if(master_)
        {
            master_.reset();
        }

        RedisSvrCliPtrVect ().swap(slave_);
    }

    RedisMasterSlaveNode(const RedisMasterSlaveNode& that)
        :master_(nullptr)
        ,slave_()
    {
       *this = that;
    }

    RedisMasterSlaveNode& operator=(const RedisMasterSlaveNode& that)
    {
        if(this == &that) return *this;

        master_ = that.master_;
        slave_ = that.slave_;

        return *this;
    }
    RedisSvrCliPtr master_;
    RedisSvrCliPtrVect slave_;
};

typedef std::shared_ptr<RedisMasterSlaveNode> RedisMasterSlaveNodePtr;

class RedisClusterNode
{
public:
    RedisClusterNode()
        :slotStart(0)
        ,slotEnd(0)
        ,clusterNode_(nullptr)
    {
    }

    RedisClusterNode(const RedisClusterNode& that)
        :slotStart(0)
        ,slotEnd(0)
        ,clusterNode_(nullptr)
    {
        *this = that;
    }

    RedisClusterNode& operator=(const RedisClusterNode& that)
    {
        if(this == &that) return *this;

        slotStart = that.slotStart;
        slotEnd = that.slotEnd;
        clusterNode_ = that.clusterNode_;

        return *this;
    }

    uint16_t slotStart;
    uint16_t slotEnd;
    RedisMasterSlaveNodePtr clusterNode_;
};

typedef std::shared_ptr<RedisClusterNode> RedisClusterNodePtr;
typedef std::vector<RedisClusterNodePtr> RedisClusterNodePtrVect;
}
#endif // end __REDIS_CLIENT_ASYNC_H__