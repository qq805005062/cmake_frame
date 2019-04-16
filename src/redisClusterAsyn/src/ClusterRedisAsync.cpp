
#include <string.h>

#include "Incommon.h"
#include "Thread.h"
#include "ThreadPool.h"
#include "RedisClient.h"
#include "LibeventIo.h"
#include "../ClusterRedisAsync.h"

namespace CLUSTER_REDIS_ASYNC
{

static std::string redisSvrInfo;
static std::unique_ptr<common::ThreadPool> eventIoPoolPtr(nullptr);
static RedisSvrInfoPtrVect inRedisClusterSvrInfo;
static size_t inRedisSvrInfoIndex = 0;
static std::vector<common::LibeventIoPtr> libeventIoPtrVect;

ClusterRedisAsync::ClusterRedisAsync()
    :connNum_(0)
    ,ioThreadNum_(0)
    ,initCb_(nullptr)
{
    PDEBUG("ClusterRedisAsync init");
}

ClusterRedisAsync::~ClusterRedisAsync()
{
    PERROR("~ClusterRedisAsync exit");
}

int ClusterRedisAsync::redisAsyncInit(int threadNum, const std::string& ipPortInfo, InitCallback& initcb, int connNum)
{
    if(connNum_ && ioThreadNum_)
    {
        return 0;
    }

    if(ipPortInfo.empty())
    {
        return -1;
    }

    eventIoPoolPtr.reset(new common::ThreadPool("eventIo"));
    if(eventIoPoolPtr == nullptr)
    {
        PERROR("redisAsyncInit thread pool new error");
        return -2;
    }

    eventIoPoolPtr->start(threadNum);

    redisSvrInfo.assign(ipPortInfo);
    connNum_ = connNum;
    ioThreadNum_ = threadNum;
    initCb_ = initcb;

    for(int i = 0; i < threadNum; i++)
    {
        common::LibeventIoPtr tmpEventIo(new common::LibeventIo());
        if(tmpEventIo == nullptr)
        {
            return -2;
        }
        libeventIoPtrVect.push_back(tmpEventIo);
    }

    eventIoPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::ClusterRedisAsync::redisAsyncConnect, this));
    return 0;
}

void ClusterRedisAsync::redisAsyncConnect()
{
    size_t len = 0;
    int errirRet = 0;
    const char *pBegin = redisSvrInfo.c_str(), *pComma = NULL, *pColon = NULL;

    do{
        pComma = utilFristConstchar(pBegin, ',');

        pColon = utilFristConstchar(pBegin, ':');
        if(pColon == NULL)
        {
            errirRet = -1;//ip port格式不正确
            break;
        }

        len = pColon - pBegin;
        if(len > MAX_IP_ADDR_LEN)
        {
            errirRet = -1;//ip port格式不正确
            break;
        }
        std::string ipAddr(pBegin, len);
        pColon++;
        if(pComma)
        {
            len = pComma - pColon;
        }else{
            len = strlen(pColon);
        }
        if(len > MAX_PORT_NUM_LEN)
        {
            errirRet = -1;//ip port格式不正确
            break;
        }
        int port = atoi(pColon);

        RedisSvrInfoPtr tmpSvrInfo(new RedisSvrInfo(port, ipAddr));
        if(tmpSvrInfo == nullptr)
        {
            errirRet = -2;//内存分配失败
            break;
        }
        inRedisClusterSvrInfo.push_back(tmpSvrInfo);
        if(pComma)
        {
            pBegin = pComma + 1;
        }
    }while(pComma);

    if(errirRet)
    {
        connNum_ = 0;
        ioThreadNum_ = 0;
        asyncInitCallBack(errirRet);
        return;
    }

    REDIS_ASYNC_CLIENT::RedisClientPtr redisClientP(new REDIS_ASYNC_CLIENT::RedisClient(0, inRedisClusterSvrInfo[inRedisSvrInfoIndex]->ipAddr_, inRedisClusterSvrInfo[inRedisSvrInfoIndex]->port_));
    if(redisClientP == nullptr)
    {
        errirRet = -2;
        asyncInitCallBack(errirRet);
    }

    common::OrderNodePtr node(new common::OrderNode(redisClientP));
    libeventIoPtrVect[0]->libeventIoOrder(node);

    for(int i = 0; i < ioThreadNum_; i++)
    {
        eventIoPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::ClusterRedisAsync::libeventIoThread, this, i));
    }
}


void ClusterRedisAsync::libeventIoThread(int index)
{
    int ret = 0;
    size_t subIndex = static_cast<size_t>(index);
    while(1)
    {
        if(libeventIoPtrVect[subIndex])
        {
            ret = libeventIoPtrVect[subIndex]->libeventIoReady();
            PERROR("LibeventIoPtrVect[index]->asyncCurlReady ret %d", ret);
        }else{
            PERROR("LibeventTcpCli new io Thread object error");
        }

        libeventIoPtrVect[subIndex].reset(new common::LibeventIo());
    }

    return;
}
void ClusterRedisAsync::asyncInitCallBack(int ret)
{
    if(initCb_)
    {
        switch(ret)
        {
            case 0:
                {
                    initCb_(ret, "");
                    break;
                }
            case -1:
                {
                    initCb_(ret, "parameter in error");
                    break;
                }
            case -2:
                {
                    initCb_(ret, "inside malloc error");
                    break;
                }
            default:
                {
                    initCb_(ret, "unkown error");
                    break;
                }
        }
    }
}

int ClusterRedisAsync::set(const std::string& key, const std::string& value, int outSecond, const CmdResultCallback& cb, void *priv)
{
    return 0;
}

int ClusterRedisAsync::redisAsyncCommand(const CmdResultCallback& cb, int outSecond, void *priv, const char *format, ...)
{
    return 0;
}

}

