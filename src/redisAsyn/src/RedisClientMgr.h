#ifndef __REDIS_CLIENT_MANAGER_H__
#define __REDIS_CLIENT_MANAGER_H__

#include <vector>
#include <map>

#include "MutexLock.h"

//#include "RedisClientMgr.h"
#include "Incommon.h"
#include "RedisClient.h"

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

        VectRedisClientPtr ().swap(slave_);
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
    RedisClientPtr master_;
    VectRedisClientPtr slave_;
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
        PTRACE("~RedisClusterNode exit");
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

typedef std::map<uint16_t, RedisClientPtr>  SlotCliInfoMap;

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
        PTRACE("~RedisClusterInfo exit");
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
        ,mutex_()
    {
        PTRACE("RedisClientMgr init");
    }

    ~RedisClientMgr()
    {
        PTRACE("~RedisClientMgr exit");
        if(nodeCli_)
        {
            PTRACE("nodeCli_.reset();");
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
        ,mutex_()
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
    int initState_;//0:初始化状态 1:正常运行状态 2:部分异常状态 3:完全失效状态，要注意区分部分异常状态，和完全失效状态，完全失效是服务不可以用，部分异常。有部分功能正常 4:资源释放，资源已经被释放
    size_t initSvrIndex_;//入口传递多个地址信息进来的时候，从第几个下标开始初始化，万一初始化失败，要依次往后连接
    size_t mgrFd_;//VECT 管理vect的下标
    size_t masterConn_;//主节点连接成功点,除了单点redis服务不使用，其他都是要使用的
    size_t slaveConn_;//从节点连接成功点，除了单点redis服务不使用，其他都是要使用的
    std::string inSvrInfoStr_;//传参进入的redis服务地址信息

    VectRedisSvrInfoPtr initSvrInfo_;//传参进来的redis地址信息解析出来的vect
    RedisClientPtr nodeCli_;
    RedisMasterSlavePtr mSlaveCli_;
    RedisClusterInfoPtr cluterCli_;

    common::MutexLock mutex_;
};

typedef std::shared_ptr<RedisClientMgr> RedisClientMgrPtr;
typedef std::vector<RedisClientMgrPtr> VectRedisClientMgrPtr;

}

#define ANALYSIS_STRING_FORMATE_ERROR           (-1)
#define ANALYSIS_MALLOC_NULLPTR_ERROR           (-2)
int analysisClusterNodes
            (const std::string& clusterNodes, REDIS_ASYNC_CLIENT::RedisClientMgr& clusterMgr, common::AtomicUInt32& ioIndex,
            REDIS_ASYNC_CLIENT::VectRedisClientPtr& addRedisCli, int ioThreadNum, int keepSecond, int connOutSecond);

bool isSelfSvrInfo(const REDIS_ASYNC_CLIENT::RedisSvrInfoPtr& svrInfo, const REDIS_ASYNC_CLIENT::RedisSvrInfoPtr& tmpInfo);

bool isSelfSvrInfo(const REDIS_ASYNC_CLIENT::RedisSvrInfoPtr& svrInfo, const std::string& ipaddr, int port);

bool isSelfSvrInfo(const REDIS_ASYNC_CLIENT::RedisClientPtr& redisCli, const std::string& ipaddr, int port);

bool isSelfSvrInfo(const REDIS_ASYNC_CLIENT::RedisClientPtr& redisCli, const REDIS_ASYNC_CLIENT::RedisSvrInfoPtr& tmpInfo);

REDIS_ASYNC_CLIENT::RedisClientPtr getRedisClient(REDIS_ASYNC_CLIENT::VectRedisClientPtr& slaveInfo, const std::string& ipaddr, int port);

REDIS_ASYNC_CLIENT::RedisClientPtr getRedisClient(REDIS_ASYNC_CLIENT::RedisMasterSlavePtr& mSlaveInfo, const std::string& ipaddr, int port);

REDIS_ASYNC_CLIENT::RedisClientPtr getRedisClient(REDIS_ASYNC_CLIENT::RedisClusterNodePtr& clusterInfo, const std::string& ipaddr, int port);

REDIS_ASYNC_CLIENT::RedisClientPtr getRedisClient(REDIS_ASYNC_CLIENT::RedisClusterInfoPtr& srcCluInfo, const std::string& ipaddr, int port);

REDIS_ASYNC_CLIENT::RedisClientPtr getRedisClient(REDIS_ASYNC_CLIENT::RedisClientMgrPtr& clusterMgr, uint16_t slot);

#endif
