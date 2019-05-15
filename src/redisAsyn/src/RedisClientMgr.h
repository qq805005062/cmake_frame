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
class SlotsInfo
{
public:
    SlotsInfo()
        :slotStart_(0)
        ,slotEnd_(0)
    {
    }

    ~SlotsInfo()
    {
    }

    SlotsInfo(const SlotsInfo& that)
        :slotStart_(0)
        ,slotEnd_(0)
    {
        *this = that;
    }

    SlotsInfo& operator=(const SlotsInfo& that)
    {
        if(this == &that) return *this;

        slotStart_ = that.slotStart_;
        slotEnd_ = that.slotEnd_;

        return *this;
    }
    
    uint16_t slotStart_;
    uint16_t slotEnd_;
};

typedef std::shared_ptr<SlotsInfo> SlotsInfoPtr;

typedef std::vector<SlotsInfoPtr> VectSlotsInfoPtr;

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
        :nodeIds_()
        ,vectSlotInfo_()
        ,clusterNode_(nullptr)
    {
    }

    RedisClusterNode(const RedisClusterNode& that)
        :nodeIds_()
        ,vectSlotInfo_()
        ,clusterNode_(nullptr)
    {
        *this = that;
    }

    RedisClusterNode& operator=(const RedisClusterNode& that)
    {
        if(this == &that) return *this;

        nodeIds_ = that.nodeIds_;
        vectSlotInfo_ = that.vectSlotInfo_;
        clusterNode_ = that.clusterNode_;

        return *this;
    }

    ~RedisClusterNode()
    {
        if(clusterNode_)
        {
            clusterNode_.reset();
        }

        VectSlotsInfoPtr ().swap(vectSlotInfo_);
    }

    std::string nodeIds_;
    VectSlotsInfoPtr vectSlotInfo_;
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
        ,masterConn_(0)
        ,slaveConn_(0)
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
        ,masterConn_(0)
        ,slaveConn_(0)
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
        masterConn_ = that.masterConn_;
        slaveConn_ = that.slaveConn_;
        inSvrInfoStr_ = that.inSvrInfoStr_;
        initSvrInfo_ = that.initSvrInfo_;
        nodeCli_ = that.nodeCli_;
        mSlaveCli_ = that.mSlaveCli_;
        cluterCli_ = that.cluterCli_;

        return *this;
    }

    int svrType_;//0:未知服务类型 1:单点redis服务 2:主从redis服务 3:redis集群服务
    int initState_;//0:初始化状态 1:正常运行状态 2:部分异常状态 3:完全失效状态，要注意区分部分异常状态，和完全失效状态，完全失效是服务不可以用，部分异常。有部分功能正常
    size_t initSvrIndex_;//入口传递多个地址信息进来的时候，从第几个下标开始初始化，万一初始化失败，要依次往后连接
    size_t mgrFd_;//VECT 管理vect的下标
    size_t masterConn_;//主节点连接成功点,除了单点redis服务不使用，其他都是要使用的
    size_t slaveConn_;//从节点连接成功点，除了单点redis服务不使用，其他都是要使用的
    std::string inSvrInfoStr_;//传参进入的redis服务地址信息

    VectRedisSvrInfoPtr initSvrInfo_;//传参进来的redis地址信息解析出来的vect
    RedisSvrCliPtr nodeCli_;
    RedisMasterSlavePtr mSlaveCli_;
    RedisClusterInfoPtr cluterCli_;
};

typedef std::shared_ptr<RedisClientMgr> RedisClientMgrPtr;
typedef std::vector<RedisClientMgrPtr> VectRedisClientMgrPtr;

}
#endif
