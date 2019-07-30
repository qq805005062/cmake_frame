#include <stdarg.h>
#include <string.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#include <async.h>

#include "Atomic.h"
#include "MutexLock.h"

#include "Incommon.h"
#include "OrderInfo.h"
#include "LibeventIo.h"
#include "RedisClientMgr.h"
#include "../RedisAsync.h"

REDIS_ASYNC_CLIENT::RedisSvrInfoPtr analysisSvrInfo(const std::string& svrInfoStr)
{
    if(svrInfoStr.empty())
    {
        return REDIS_ASYNC_CLIENT::RedisSvrInfoPtr();
    }

    std::string::size_type pos = svrInfoStr.find(':');
    if (pos == std::string::npos)
    {
        return REDIS_ASYNC_CLIENT::RedisSvrInfoPtr();
    }

    std::string portStr = svrInfoStr.substr(pos + 1);
    int tmpPort  = atoi(portStr.c_str());
    std::string tmpIpStr = svrInfoStr.substr(0, pos);
    PDEBUG("redis svr info %s::%d", tmpIpStr.c_str(), tmpPort);

    REDIS_ASYNC_CLIENT::RedisSvrInfoPtr tmpSvr(new REDIS_ASYNC_CLIENT::RedisSvrInfo(tmpPort, tmpIpStr));
    return tmpSvr;
}

bool isSelfSvrInfo(const REDIS_ASYNC_CLIENT::RedisSvrInfoPtr& svrInfo, const REDIS_ASYNC_CLIENT::RedisSvrInfoPtr& tmpInfo)
{
    if(svrInfo && tmpInfo && ((*svrInfo) == (*tmpInfo)))
    {
        return true;
    }

    return false;
}

bool isSelfSvrInfo(const REDIS_ASYNC_CLIENT::RedisSvrInfoPtr& svrInfo, const std::string& ipaddr, int port)
{
    if(svrInfo && (svrInfo->ipAddr_.compare(ipaddr) == 0) && (svrInfo->port_ == port))
    {
        return true;
    }
    
    return false;
}

bool isSelfSvrInfo(const REDIS_ASYNC_CLIENT::RedisClientPtr& redisCli, const std::string& ipaddr, int port)
{
    if(redisCli && isSelfSvrInfo(redisCli->redisCliSvrInfo(), ipaddr, port))
    {
        return true;
    }
    
    return false;
}

bool isSelfSvrInfo(const REDIS_ASYNC_CLIENT::RedisClientPtr& redisCli, const REDIS_ASYNC_CLIENT::RedisSvrInfoPtr& tmpInfo)
{
    if(redisCli)
    {
        return isSelfSvrInfo(redisCli->redisCliSvrInfo(), tmpInfo);
    }
    return false;
}


REDIS_ASYNC_CLIENT::RedisClientPtr getRedisClient(REDIS_ASYNC_CLIENT::VectRedisClientPtr& slaveInfo, const std::string& ipaddr, int port)
{
    for(size_t i = 0; i < slaveInfo.size(); i++)
    {
        if(isSelfSvrInfo(slaveInfo[i], ipaddr, port))
        {
            return slaveInfo[i];
        }
    }

    return REDIS_ASYNC_CLIENT::RedisClientPtr();
}

REDIS_ASYNC_CLIENT::RedisClientPtr getRedisClient(REDIS_ASYNC_CLIENT::RedisMasterSlavePtr& mSlaveInfo, const std::string& ipaddr, int port)
{
    if(mSlaveInfo == nullptr)
    {
        return REDIS_ASYNC_CLIENT::RedisClientPtr();
    }

    if(isSelfSvrInfo(mSlaveInfo->master_, ipaddr, port))
    {
        REDIS_ASYNC_CLIENT::RedisClientPtr tmp = mSlaveInfo->master_;
        return tmp;
    }

    return getRedisClient(mSlaveInfo->slave_, ipaddr, port);
}

REDIS_ASYNC_CLIENT::RedisClientPtr getRedisClient(REDIS_ASYNC_CLIENT::RedisClusterNodePtr& clusterNodeInfo, const std::string& ipaddr, int port)
{
    if(clusterNodeInfo)
    {
        return getRedisClient(clusterNodeInfo->clusterNode_, ipaddr, port);
    
}

    return REDIS_ASYNC_CLIENT::RedisClientPtr();
}

REDIS_ASYNC_CLIENT::RedisClientPtr getRedisClient(REDIS_ASYNC_CLIENT::RedisClusterInfoPtr& clusterInfo, const std::string& ipaddr, int port)
{
    if(clusterInfo == nullptr)
    {
        return REDIS_ASYNC_CLIENT::RedisClientPtr();
    }

    for(size_t i = 0; i < clusterInfo->clusterVectInfo_.size(); i++)
    {
        REDIS_ASYNC_CLIENT::RedisClientPtr tmp = getRedisClient(clusterInfo->clusterVectInfo_[i], ipaddr, port);
        if(tmp)
        {
            return tmp;
        }
    }
    
    return REDIS_ASYNC_CLIENT::RedisClientPtr();
}

//一般调用这个接口都是在找对应的从节点了。一般都是在主节点挂了之后才会调用这个接口来暂时解决业务
//这个接口是获取对应主节点的从节点
REDIS_ASYNC_CLIENT::RedisClientPtr getRedisClient(REDIS_ASYNC_CLIENT::RedisClientMgrPtr& clusterMgr, uint16_t slot)
{
    if(clusterMgr == nullptr || clusterMgr->cluterCli_ == nullptr)
    {
        return REDIS_ASYNC_CLIENT::RedisClientPtr();
    }

    bool slaveFound = false;
    for(size_t i = 0; i < clusterMgr->cluterCli_->clusterVectInfo_.size(); i++)
    {
        if(clusterMgr->cluterCli_->clusterVectInfo_[i])
        {
            for(size_t j = 0; j < clusterMgr->cluterCli_->clusterVectInfo_[i]->vectSlotInfo_.size(); j++)
            {
                if(clusterMgr->cluterCli_->clusterVectInfo_[i]->vectSlotInfo_[j] &&
                    (slot >= clusterMgr->cluterCli_->clusterVectInfo_[i]->vectSlotInfo_[j]->slotStart_) &&
                    (slot <= clusterMgr->cluterCli_->clusterVectInfo_[i]->vectSlotInfo_[j]->slotEnd_))
                {
                    slaveFound = true;
                    break;
                }
            }

            if(slaveFound && clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_)
            {
                for(size_t k = 0; k < clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_.size(); k++)
                {
                    if(clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_[k] &&
                        (clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_[k]->tcpCliState() == REDIS_CLIENT_STATE_CONN))
                    {
                        return clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_[k];
                    
}
                
}

                return REDIS_ASYNC_CLIENT::RedisClientPtr();
            }
        }
    
}
    return REDIS_ASYNC_CLIENT::RedisClientPtr();
}


/*
9be7650002bdbfd6b099c07969c6969d120b2b6c 192.169.6.211:6790@16790 master - 0 1557940444069 5 connected 10923-16383
af805d65d9ee40af96695a213b300afd2f5e24d6 192.169.6.234:6791@16791 slave a4892e9a69d9e2858a9d0f45a915c40734b8a4aa 0 1557940443067 3 connected
1adf947915d6f3f4bc9943831734d34b7e1fc610 192.169.6.234:6790@16790 myself,slave 19b1e55b4b30bb9c48b504121f36bd253fe7d30f 0 1557940442000 1 connected
37e495750f36270d3a55a41a126f940316c54c8c 192.169.6.211:6791@16791 slave 9be7650002bdbfd6b099c07969c6969d120b2b6c 0 1557940446071 6 connected
a4892e9a69d9e2858a9d0f45a915c40734b8a4aa 192.169.6.233:6790@16790 master - 0 1557940441000 3 connected 5461-10922
19b1e55b4b30bb9c48b504121f36bd253fe7d30f 192.169.6.233:6791@16791 master - 0 1557940445070 7 connected 0-5460
*/
/*
 * [analysisClusterNodes] 此方法之负责解析解群对应cluster nodes字符串。进这个方法之前要上锁，出这个方法，如果正常的话，要判断redis集群状态。
                            方法内部如果返回正常值会把redis集群指针直接替换掉
                            此方法应该可以应对redis增加、删除节点信息情况
                            没有主节点的从信息会被丢弃
 * @author xiaoxiao 2019-05-24
 * @param clusterNodes 服务端返回的string信息
 * @param clusterMgr redis集群信息管理对象，不可以为空
 * @param ioIndex io的原子变量引用
 * @param addRedisCli 外部需要增加连接的
 * @param ioThreadNum io线程数
 * @param keepSecond 保持活跃的时间
 * @param connOutSecond 连接超时时间
 *
 * @return 0 success, -1 string formate error, -2 malloc error
 */
int analysisClusterNodes
            (const std::string& clusterNodes, REDIS_ASYNC_CLIENT::RedisClientMgr& clusterMgr, common::AtomicUInt32& ioIndex,
            REDIS_ASYNC_CLIENT::VectRedisClientPtr& addRedisCli, int ioThreadNum, int keepSecond, int connOutSecond)
{
    REDIS_ASYNC_CLIENT::RedisClusterInfoPtr tmpClusterInfo(nullptr);
    size_t masterConn = 0, slaveConn = 0;
    if(clusterNodes.empty())
    {
        return ANALYSIS_STRING_FORMATE_ERROR;
    }
    
    std::vector<std::string> vLines;
    split(clusterNodes, "\n", vLines);
    for (size_t i = 0; i < vLines.size(); ++i)
    {
        std::vector<std::string> nodeInfo;
        split(vLines[i], " ", nodeInfo);
        if (nodeInfo.size() < 8)
        {
            return ANALYSIS_STRING_FORMATE_ERROR;
        }

        if(strstr(nodeInfo[2].c_str(), "master"))
        {
            if (nodeInfo.size() < 9)
            {
                return ANALYSIS_STRING_FORMATE_ERROR;
            }
            
            REDIS_ASYNC_CLIENT::RedisSvrInfoPtr masterInfo = analysisSvrInfo(nodeInfo[1]);
            if (masterInfo == nullptr)
            {
                return ANALYSIS_STRING_FORMATE_ERROR;
            }
            
            REDIS_ASYNC_CLIENT::RedisClientPtr tmpCli(nullptr);
            if(clusterMgr.cluterCli_)
            {
                #if 1
                tmpCli = getRedisClient(clusterMgr.cluterCli_, masterInfo->ipAddr_, masterInfo->port_);
                #else
                for(size_t j = 0; j < clusterMgr.cluterCli_->clusterVectInfo_.size(); j++)
                {
                    if(clusterMgr.cluterCli_->clusterVectInfo_[j] && clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_)
                    {
                        if(clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->master_ && isSelfSvrInfo(clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->master_, masterInfo))
                        {
                            tmpCli = clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->master_;
                            break;
                        }

                        for(size_t k = 0; k < clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->slave_.size(); k++)
                        {
                            if(clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->slave_[k] && isSelfSvrInfo(clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->slave_[k], masterInfo))
                            {
                                tmpCli = clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->slave_[k];
                                break;
                            }
                        }
                    }

                    if(tmpCli)
                    {
                        break;
                    }
                }
                #endif
            }else if(clusterMgr.nodeCli_)
            {
                if(isSelfSvrInfo(clusterMgr.nodeCli_, masterInfo))
                {
                    tmpCli = clusterMgr.nodeCli_;
                }
            }

            if(tmpCli == nullptr)
            {
                size_t tmpIoIndex = ioIndex.incrementAndGet();
                tmpIoIndex = tmpIoIndex % ioThreadNum;
                tmpCli.reset(new REDIS_ASYNC_CLIENT::RedisClient(tmpIoIndex, clusterMgr.mgrFd_, masterInfo, keepSecond, connOutSecond));
                if(tmpCli == nullptr)
                {
                    return ANALYSIS_MALLOC_NULLPTR_ERROR;
                }

                addRedisCli.push_back(tmpCli);
            }
            if(tmpCli->tcpCliState() == REDIS_CLIENT_STATE_CONN)
            {
                masterConn++;
            }

            REDIS_ASYNC_CLIENT::RedisMasterSlavePtr tmpMS(new REDIS_ASYNC_CLIENT::RedisMasterSlave());
            if(tmpMS == nullptr)
            {
                return ANALYSIS_MALLOC_NULLPTR_ERROR;
            }
            tmpMS->master_ = tmpCli;

            REDIS_ASYNC_CLIENT::RedisClusterNodePtr tmpClusNode(new REDIS_ASYNC_CLIENT::RedisClusterNode());
            if(tmpClusNode == nullptr)
            {
                return ANALYSIS_MALLOC_NULLPTR_ERROR;
            }
            tmpClusNode->clusterNode_ = tmpMS;
            tmpClusNode->nodeIds_.assign(nodeInfo[0]);

            for(size_t kk = 8; kk < nodeInfo.size(); kk++)
            {
                REDIS_ASYNC_CLIENT::SlotsInfoPtr tmpSlots(new REDIS_ASYNC_CLIENT::SlotsInfo());
                if(tmpSlots == nullptr)
                {
                    return ANALYSIS_MALLOC_NULLPTR_ERROR;
                }
                std::string::size_type pos = nodeInfo[kk].find('-');
                if (pos == std::string::npos)
                {
                    tmpSlots->slotStart_ = static_cast<uint16_t>(atoi(nodeInfo[kk].c_str()));
                    tmpSlots->slotEnd_ = tmpSlots->slotStart_;
                }
                else
                {
                    const std::string slotEndStr = nodeInfo[kk].substr(pos + 1);
                    tmpSlots->slotEnd_ = static_cast<uint16_t>(atoi(slotEndStr.c_str()));
                    tmpSlots->slotStart_ = static_cast<uint16_t>(atoi(nodeInfo[kk].substr(0, pos).c_str()));
                }
                tmpClusNode->vectSlotInfo_.push_back(tmpSlots);
            }

            if(tmpClusterInfo == nullptr)
            {
                tmpClusterInfo.reset(new REDIS_ASYNC_CLIENT::RedisClusterInfo());
                if(tmpClusterInfo == nullptr)
                {
                    return ANALYSIS_MALLOC_NULLPTR_ERROR;
                }
            }
            tmpClusterInfo->clusterVectInfo_.push_back(tmpClusNode);

            for(size_t jjj = 0; jjj < tmpClusNode->vectSlotInfo_.size(); ++jjj)
            {
                for(uint16_t jj = tmpClusNode->vectSlotInfo_[jjj]->slotStart_; jj <= tmpClusNode->vectSlotInfo_[jjj]->slotEnd_; jj++)
                {
                    tmpClusterInfo->clusterSlotMap_.insert(REDIS_ASYNC_CLIENT::SlotCliInfoMap::value_type(jj, tmpCli));
                }
            }

            for (size_t jj = 0; jj < vLines.size(); ++jj)
            {
                std::vector<std::string> tmpSalveInfo;
                split(vLines[jj], " ", tmpSalveInfo);
                if (tmpSalveInfo.size() < 8)
                {
                    return ANALYSIS_STRING_FORMATE_ERROR;
                }

                if(nodeInfo[0].compare(tmpSalveInfo[3]) == 0)
                {
                    REDIS_ASYNC_CLIENT::RedisSvrInfoPtr slaveInfo = analysisSvrInfo(tmpSalveInfo[1]);
                    if (slaveInfo == nullptr)
                    {
                        return ANALYSIS_STRING_FORMATE_ERROR;
                    }

                    tmpCli.reset();
                    if(clusterMgr.cluterCli_)
                    {
                        #if 1
                        tmpCli = getRedisClient(clusterMgr.cluterCli_, slaveInfo->ipAddr_, slaveInfo->port_);
                        #else
                        for(size_t j = 0; j < clusterMgr.cluterCli_->clusterVectInfo_.size(); j++)
                        {
                            if(clusterMgr.cluterCli_->clusterVectInfo_[j] && clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_)
                            {
                                if(clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->master_ && isSelfSvrInfo(clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->master_, slaveInfo))
                                {
                                    tmpCli = clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->master_;
                                    break;
                                }

                                for(size_t k = 0; k < clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->slave_.size(); k++)
                                {
                                    if(clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->slave_[k] && isSelfSvrInfo(clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->slave_[k], slaveInfo))
                                    {
                                        tmpCli = clusterMgr.cluterCli_->clusterVectInfo_[j]->clusterNode_->slave_[k];
                                        break;
                                    }
                                }
                            }

                            if(tmpCli)
                            {
                                break;
                            }
                        }
                        #endif
                    }else if(clusterMgr.nodeCli_)
                    {
                        if(isSelfSvrInfo(clusterMgr.nodeCli_, slaveInfo))
                        {
                            tmpCli = clusterMgr.nodeCli_;
                        }
                    }

                    if(tmpCli == nullptr)
                    {
                        size_t tmpIoIndex = ioIndex.incrementAndGet();
                        tmpIoIndex = tmpIoIndex % ioThreadNum;
                        tmpCli.reset(new REDIS_ASYNC_CLIENT::RedisClient(tmpIoIndex, clusterMgr.mgrFd_, slaveInfo, keepSecond, connOutSecond));
                        if(tmpCli == nullptr)
                        {
                            return ANALYSIS_MALLOC_NULLPTR_ERROR;
                        }

                        addRedisCli.push_back(tmpCli);
                    }
                    if(tmpCli->tcpCliState() == REDIS_CLIENT_STATE_CONN)
                    {
                        slaveConn++;
                    }
                    
                    tmpMS->slave_.push_back(tmpCli);
                }
            }
        }
    }

    clusterMgr.cluterCli_ = tmpClusterInfo;
    clusterMgr.masterConn_ = masterConn;
    clusterMgr.slaveConn_ = slaveConn;
    return 0;
}

/*
# Replication
role:master
connected_slaves:2
slave0:ip=192.169.6.234,port=8000,state=online,offset=781713,lag=0
slave1:ip=192.169.6.233,port=8000,state=online,offset=781431,lag=1
master_repl_offset:781713
repl_backlog_active:1
repl_backlog_size:1048576
repl_backlog_first_byte_offset:2
repl_backlog_histlen:781712

# Replication
role:slave
master_host:192.169.6.211
master_port:8000
master_link_status:up
master_last_io_seconds_ago:1
master_sync_in_progress:0
slave_repl_offset:845738
slave_priority:100
slave_read_only:1
connected_slaves:0
master_repl_offset:0
repl_backlog_active:0
repl_backlog_size:1048576
repl_backlog_first_byte_offset:0
repl_backlog_histlen:0
 */
int analysisSlaveNodes
            (const std::string& clusterNodes, REDIS_ASYNC_CLIENT::RedisClientMgr& clusterMgr, common::AtomicUInt32& ioIndex,
            REDIS_ASYNC_CLIENT::VectRedisClientPtr& addRedisCli, int ioThreadNum, int keepSecond, int connOutSecond)
{
    int result = 0;
    size_t masterConn = 0, slaveConn = 0;
    if(clusterNodes.empty())
    {
        return ANALYSIS_STRING_FORMATE_ERROR;
    }

    REDIS_ASYNC_CLIENT::RedisMasterSlavePtr mSlaveInfo(new REDIS_ASYNC_CLIENT::RedisMasterSlave());
    if(mSlaveInfo == nullptr)
    {
        return ANALYSIS_MALLOC_NULLPTR_ERROR;
    }
    REDIS_ASYNC_CLIENT::RedisClientPtr tmpCli(nullptr);
    std::vector<std::string> vLines;
    split(clusterNodes, "\n", vLines);

    PDEBUG("vLines[1] %s", vLines[1].c_str());
    std::string::size_type pos = vLines[1].find("role:master");
    if(pos != std::string::npos)
    {
        mSlaveInfo->master_ = clusterMgr.nodeCli_;
        if(mSlaveInfo->master_->tcpCliState() == REDIS_CLIENT_STATE_CONN)
        {
            masterConn++;
        }
        PDEBUG("vLines[2] %s\n", vLines[2].c_str());
        pos = vLines[2].find(':');
        if (pos == std::string::npos)
        {
            return ANALYSIS_STRING_FORMATE_ERROR;
        }

        std::string slaveNumStr = vLines[2].substr(pos + 1);
        int slaveNum = atoi(slaveNumStr.c_str());
        PDEBUG("slaveNum %d", slaveNum);
        for(int i = 0; i < slaveNum; i++)
        {
            size_t index = 3 + static_cast<size_t>(i);
            PDEBUG("vLines[%ld] %s\n", index, vLines[index].c_str());
            std::string::size_type beginPos = vLines[index].find('=');
            if(beginPos == std::string::npos)
            {
                return ANALYSIS_STRING_FORMATE_ERROR;
            }
            PDEBUG("vLines[%ld] %s\n", index, vLines[index].c_str());
            std::string::size_type endPos = vLines[index].find(',');
            if(endPos == std::string::npos)
            {
                return ANALYSIS_STRING_FORMATE_ERROR;
            }

            std::string ipaddr = vLines[index].substr((beginPos + 1), (endPos - beginPos - 1));
            PDEBUG("ipaddr %s\n", ipaddr.c_str());
            beginPos = vLines[index].find('=', endPos);
            if(beginPos == std::string::npos)
            {
                return ANALYSIS_STRING_FORMATE_ERROR;
            }
            endPos = vLines[index].find(',', beginPos);
            if(endPos == std::string::npos)
            {
                return ANALYSIS_STRING_FORMATE_ERROR;
            }
            std::string portStr = vLines[index].substr((beginPos + 1), (endPos - beginPos - 1));
            int tmpPort  = atoi(portStr.c_str());
            PDEBUG("slave redis svr info %s::%d\n", ipaddr.c_str(), tmpPort);

            tmpCli = getRedisClient(clusterMgr.mSlaveCli_, ipaddr, tmpPort);
            if(tmpCli == nullptr)
            {
                REDIS_ASYNC_CLIENT::RedisSvrInfoPtr tmpSvr(new REDIS_ASYNC_CLIENT::RedisSvrInfo(ipaddr, tmpPort));
                if(tmpSvr == nullptr)
                {
                    return ANALYSIS_MALLOC_NULLPTR_ERROR;
                }
                
                size_t tmpIoIndex = ioIndex.incrementAndGet();
                tmpIoIndex = tmpIoIndex % ioThreadNum;
                tmpCli.reset(new REDIS_ASYNC_CLIENT::RedisClient(tmpIoIndex, clusterMgr.mgrFd_, tmpSvr, keepSecond, connOutSecond));
                if(tmpCli == nullptr)
                {
                    return ANALYSIS_MALLOC_NULLPTR_ERROR;
                }

                addRedisCli.push_back(tmpCli);
            }
            if(tmpCli->tcpCliState() == REDIS_CLIENT_STATE_CONN)
            {
                slaveConn++;
            }
            mSlaveInfo->slave_.push_back(tmpCli);
        }
    }else{
        mSlaveInfo->slave_.push_back(clusterMgr.nodeCli_);
        if(clusterMgr.nodeCli_->tcpCliState() == REDIS_CLIENT_STATE_CONN)
        {
            slaveConn++;
        }
        PDEBUG("vLines[2] %s\n", vLines[2].c_str());
        pos = vLines[2].find(':');
        if (pos == std::string::npos)
        {
            return ANALYSIS_STRING_FORMATE_ERROR;
        }
        std::string ipaddr = vLines[2].substr(pos + 1);

        PDEBUG("vLines[3] %s\n", vLines[3].c_str());
        pos = vLines[3].find(':');
        if (pos == std::string::npos)
        {
            return ANALYSIS_STRING_FORMATE_ERROR;
        }
        std::string portStr = vLines[3].substr(pos + 1);
        int tmpPort  = atoi(portStr.c_str());
        PDEBUG("master redis svr info %s::%d\n", ipaddr.c_str(), tmpPort);
        
        tmpCli = getRedisClient(clusterMgr.mSlaveCli_, ipaddr, tmpPort);
        if(tmpCli == nullptr)
        {
            REDIS_ASYNC_CLIENT::RedisSvrInfoPtr tmpSvr(new REDIS_ASYNC_CLIENT::RedisSvrInfo(ipaddr, tmpPort));
            if(tmpSvr == nullptr)
            {
                return ANALYSIS_MALLOC_NULLPTR_ERROR;
            }
            
            size_t tmpIoIndex = ioIndex.incrementAndGet();
            tmpIoIndex = tmpIoIndex % ioThreadNum;
            tmpCli.reset(new REDIS_ASYNC_CLIENT::RedisClient(tmpIoIndex, clusterMgr.mgrFd_, tmpSvr, keepSecond, connOutSecond));
            if(tmpCli == nullptr)
            {
                return ANALYSIS_MALLOC_NULLPTR_ERROR;
            }

            addRedisCli.push_back(tmpCli);
        }
        if(tmpCli->tcpCliState() == REDIS_CLIENT_STATE_CONN)
        {
            PDEBUG("master %s::%d had been connected\n", ipaddr.c_str(), tmpPort);
            masterConn++;
            clusterMgr.nodeCli_ = tmpCli;
        }
        mSlaveInfo->master_ = tmpCli;
        result = 1;
    }

    clusterMgr.mSlaveCli_ = mSlaveInfo;
    clusterMgr.masterConn_ = masterConn;
    clusterMgr.slaveConn_ = slaveConn;
    return result;
}


//redis 集群更新连接数，主节点连接数、从节点连接数
int clusterRedisConnsUpdate(REDIS_ASYNC_CLIENT::RedisClientMgrPtr& clusterMgr, const std::string& ipaddr, int port, bool onConned, bool onCount)
{
    if(clusterMgr == nullptr || clusterMgr->cluterCli_ == nullptr)
    {
        return NO_IN_CLUSTER_ADDRINFO;
    }
    for(size_t i = 0; i < clusterMgr->cluterCli_->clusterVectInfo_.size(); i++)
    {
        if(clusterMgr->cluterCli_->clusterVectInfo_[i] &&
            clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_)
        {
            if(clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_->master_ &&
                (clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_->master_->redisSvrPort() == port) &&
                (clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_->master_->redisSvrIpaddr().compare(ipaddr) == 0))
            {
                if(onCount)
                {
                    if(onConned)
                    {
                        clusterMgr->masterConn_++;
                    }else{
                        clusterMgr->masterConn_--;
                    }
                }
                return MASTER_CLUSTER_ADDRINFO;
            }
            
            for(size_t jj = 0; jj < clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_.size(); jj++)
            {
                if(clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_[jj] &&
                    (clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_[jj]->redisSvrPort() == port) &&
                    (clusterMgr->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_[jj]->redisSvrIpaddr().compare(ipaddr) == 0))
                    {
                        if(onCount)
                        {
                            if(onConned)
                            {
                                clusterMgr->slaveConn_++;
                            }else{
                                clusterMgr->slaveConn_--;
                            }
                        }
                        return SALVE_CLUSTER_ADDRINFO;
                    }
            }
        }
    }
    return NO_IN_CLUSTER_ADDRINFO;
}

int infoReplicConnsUpdate(REDIS_ASYNC_CLIENT::RedisClientMgrPtr& clusterMgr, const std::string& ipaddr, int port, bool onConned, bool onCount)
{
    if(clusterMgr == nullptr || clusterMgr->mSlaveCli_ == nullptr)
    {
        return NO_IN_CLUSTER_ADDRINFO;
    }

    if(clusterMgr->mSlaveCli_->master_ && 
        (clusterMgr->mSlaveCli_->master_->redisSvrPort() == port)&&
        (clusterMgr->mSlaveCli_->master_->redisSvrIpaddr().compare(ipaddr) == 0))
    {
        if(onCount)
        {
            if(onConned)
            {
                clusterMgr->masterConn_++;
            }else{
                clusterMgr->masterConn_--;
            }
        }
        return MASTER_CLUSTER_ADDRINFO;
    }

    for(size_t i = 0;i < clusterMgr->mSlaveCli_->slave_.size(); i++)
    {
        if(clusterMgr->mSlaveCli_->slave_[i] &&
            (clusterMgr->mSlaveCli_->slave_[i]->redisSvrPort() == port) &&
            (clusterMgr->mSlaveCli_->slave_[i]->redisSvrIpaddr().compare(ipaddr) == 0))
        {
            if(onCount)
            {
                if(onConned)
                {
                    clusterMgr->slaveConn_++;
                }else{
                    clusterMgr->slaveConn_--;
                }
            }
            return SALVE_CLUSTER_ADDRINFO;
        }
    }
    return NO_IN_CLUSTER_ADDRINFO;
}

/*
 * [PartRedisAsync.cpp] 这个文件存在的意义就是RedisAsync.cpp文件里面内容太多了。分担部分代码过来。保证代码可读性
 * @author xiaoxiao 2019-05-22
 * @param
 * @param
 * @param
 *
 * @return
 */
namespace CLUSTER_REDIS_ASYNC
{
void RedisAsync::asyncStateCallBack(int asyFd, int stateCode, const std::string& stateMsg)
{
    if(stateCb_)
    {
        switch(stateCode)
        {
            case REDIS_SVR_INIT_STATE:
                {
                    char stateMsgBuf[1024] = {0};
                    sprintf(stateMsgBuf, "init state %s", stateMsg.c_str());
                    stateCb_(asyFd, stateCode, stateMsgBuf);
                    break;
                }
            case REDIS_SVR_RUNING_STATE:
                {
                    char stateMsgBuf[1024] = {0};
                    sprintf(stateMsgBuf, "running ok %s", stateMsg.c_str());
                    stateCb_(asyFd, stateCode, stateMsgBuf);
                    break;
                }
            case REDIS_EXCEPTION_STATE:
                {
                    char stateMsgBuf[1024] = {0};
                    sprintf(stateMsgBuf, "part exception %s", stateMsg.c_str());
                    stateCb_(asyFd, stateCode, stateMsgBuf);
                    break;
                }
            case REDIS_SVR_ERROR_STATE:
                {
                    char stateMsgBuf[1024] = {0};
                    sprintf(stateMsgBuf, "error state %s", stateMsg.c_str());
                    stateCb_(asyFd, stateCode, stateMsgBuf);
                    break;
                }
            case REDIS_SVR_INVALID_STATE:
                {
                    char stateMsgBuf[1024] = {0};
                    sprintf(stateMsgBuf, "redis svr invalid state %s", stateMsg.c_str());
                    stateCb_(asyFd, stateCode, stateMsgBuf);
                    break;
                }
            default:
                {
                    char stateMsgBuf[1024] = {0};
                    sprintf(stateMsgBuf, "unknow state %s", stateMsg.c_str());
                    stateCb_(asyFd, stateCode, stateMsgBuf);
                    break;
                }
        }
    }
}

void RedisAsync::asyncExceCallBack(int asyFd, int exceCode, const std::string& exceMsg)
{
    if(exceCb_)
    {
        switch(exceCode)
        {
            case EXCE_MALLOC_NULL:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "new or malloc nullptr %s", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
            case SVR_CONNECT_RESET:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "connect failed %s", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
            case SVR_CONECT_DISCONNECT:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "connect had been disconnect %s", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
            case CLUSTER_NODES_CHANGE:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "cluster nodes change %s", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
            case UNKOWN_REDIS_SVR_TYPE:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "redis svr type unknow %s", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
            case MASTER_SLAVE_NODES_CHANGE:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "master cluster nodes change %s", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
            case CLUSTER_INITSTR_FORMATE:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "cluster nodes str error %s", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
            case CLUSTER_INITSTR_UNKNOWN:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "cluster nodes str unkown error %s", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
            default:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "%s unkown exception", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//以下可以自定义一些接口、命令函数。
//方法入口可以不做参数校验，redisAsyncCommand方法做了统一的严格的参数校验
int RedisAsync::set(int asyFd, const std::string& key, const std::string& value, int outSecond, const CmdResultCallback& cb, void *priv)
{
    char cmdBuf[1024] = {0};

    sprintf(cmdBuf, "set %s %s", key.c_str(), value.c_str());
    return redisAsyncCommand(asyFd, cb, outSecond, priv, key, cmdBuf);
}

int RedisAsync::redisAsyncCommand(int asyFd, const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const char *format, ...)
{
    char *cmd = NULL;
    va_list ap;
    va_start(ap, format);

    int len = redisvFormatCommand(&cmd, format, ap);
    if (len == -1)
    {
     return CMD_PARAMETER_ERROR_CODE;
    }
    std::string cmdStr(cmd);
    if(cmd)
    {
     free(cmd);
    }
    va_end(ap);
    return redisAsyncCommand(asyFd, cb, outSecond, priv, key, cmdStr);;
}

}

