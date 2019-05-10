#ifndef __REDIS_CLIENT_MANAGER_H__
#define __REDIS_CLIENT_MANAGER_H__

#include <vector>
#include <map>

//#include "RedisClientMgr.h"
#include "Incommon.h"
#include "RedisClient.h"

#define REDIS_UNKNOWN_SERVER            (0)
#define REDIS_SINGLE_SERVER             (1)
#define REDIS_MASTER_SLAVE_SERVER       (2)
#define REDIS_CLUSTER_SERVER            (3)

namespace REDIS_ASYNC_CLIENT
{

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
typedef std::vector<RedisSvrCliPtr> VectRedisSvrCliPtr;

class RedisMasterSlave
{
public:
    RedisMasterSlave()
        :master_(nullptr)
        ,slave_()
    {
    }

    ~RedisMasterSlave()
    {
        if(master_)
        {
            master_.reset();
        }

        VectRedisSvrCliPtr ().swap(slave_);
    }

    RedisMasterSlave(const RedisMasterSlave& that)
        :master_(nullptr)
        ,slave_()
    {
       *this = that;
    }

    RedisMasterSlave& operator=(const RedisMasterSlave& that)
    {
        if(this == &that) return *this;

        master_ = that.master_;
        slave_ = that.slave_;

        return *this;
    }
    RedisSvrCliPtr master_;
    VectRedisSvrCliPtr slave_;
};

typedef std::shared_ptr<RedisMasterSlave> RedisMasterSlavePtr;
typedef std::vector<RedisMasterSlavePtr> VectMasterSlavePtr;

class RedisClusterNode
{
public:
    RedisClusterNode()
        :slotStart_(0)
        ,slotEnd_(0)
        ,clusterNode_(nullptr)
    {
    }

    RedisClusterNode(const RedisClusterNode& that)
        :slotStart_(0)
        ,slotEnd_(0)
        ,clusterNode_(nullptr)
    {
        *this = that;
    }

    RedisClusterNode& operator=(const RedisClusterNode& that)
    {
        if(this == &that) return *this;

        slotStart_ = that.slotStart_;
        slotEnd_ = that.slotEnd_;
        clusterNode_ = that.clusterNode_;

        return *this;
    }

    ~RedisClusterNode()
    {
        if(clusterNode_)
        {
            clusterNode_.reset();
        }
    }

    uint16_t slotStart_;
    uint16_t slotEnd_;
    RedisMasterSlavePtr clusterNode_;
};

typedef std::shared_ptr<RedisClusterNode> RedisClusterNodePtr;
typedef std::vector<RedisClusterNodePtr> VectRedisClusterNodePtr;

typedef std::map<uint16_t, RedisClusterNodePtr>  SlotCliInfoMap;

class RedisClusterInfo
{
public:
    RedisClusterInfo()
        :clusterSlotMap_()
        ,clusterVectInfo_()
    {
    }

    RedisClusterInfo(const RedisClusterInfo& that)
        :clusterSlotMap_()
        ,clusterVectInfo_()
    {
        *this = that;
    }

    RedisClusterInfo& operator=(const RedisClusterInfo& that)
    {
        if(this == &that) return *this;

        clusterSlotMap_ = that.clusterSlotMap_;
        clusterVectInfo_ = that.clusterVectInfo_;

        return *this;
    }

    ~RedisClusterInfo()
    {
        SlotCliInfoMap ().swap(clusterSlotMap_);
        VectRedisClusterNodePtr ().swap(clusterVectInfo_);
    }

    SlotCliInfoMap clusterSlotMap_;
    VectRedisClusterNodePtr clusterVectInfo_;
};
typedef std::shared_ptr<RedisClusterInfo> RedisClusterInfoPtr;

class RedisClientMgr
{
public:
    RedisClientMgr()
        :svrType_(0)
        ,initState_(0)
        ,initSvrIndex_(0)
        ,mgrFd_(0)
        ,inSvrInfoStr_()
        ,initSvrInfo_()
        ,nodeCli_(nullptr)
        ,mSlaveCli_(nullptr)
        ,cluterCli_(nullptr)
    {
    }

    ~RedisClientMgr()
    {
        if(nodeCli_)
        {
            nodeCli_.reset();
        }
        
        if(mSlaveCli_)
        {
            mSlaveCli_.reset();
        }
        
        if(cluterCli_)
        {
            cluterCli_.reset();
        }

        VectRedisSvrInfoPtr ().swap(initSvrInfo_);
    }

    RedisClientMgr(const RedisClientMgr& that)
        :svrType_(0)
        ,initState_(0)
        ,initSvrIndex_(0)
        ,mgrFd_(0)
        ,inSvrInfoStr_()
        ,initSvrInfo_()
        ,nodeCli_(nullptr)
        ,mSlaveCli_(nullptr)
        ,cluterCli_(nullptr)
    {
        *this = that;
    }

    RedisClientMgr& operator=(const RedisClientMgr& that)
    {
        if(this == &that) return *this;

        svrType_ = that.svrType_;
        initState_ = that.initState_;
        initSvrIndex_ = that.initSvrIndex_;
        mgrFd_ = that.mgrFd_;
        inSvrInfoStr_ = that.inSvrInfoStr_;
        initSvrInfo_ = that.initSvrInfo_;
        nodeCli_ = that.nodeCli_;
        mSlaveCli_ = that.mSlaveCli_;
        cluterCli_ = that.cluterCli_;

        return *this;
    }

    int svrType_;//0 1 2 3
    int initState_;//0 1 2
    size_t initSvrIndex_;
    size_t mgrFd_;
    std::string inSvrInfoStr_;

    VectRedisSvrInfoPtr initSvrInfo_;
    RedisSvrCliPtr nodeCli_;
    RedisMasterSlavePtr mSlaveCli_;
    RedisClusterInfoPtr cluterCli_;
};

typedef std::shared_ptr<RedisClientMgr> RedisClientMgrPtr;
typedef std::vector<RedisClientMgrPtr> VectRedisClientMgrPtr;

}
#endif
