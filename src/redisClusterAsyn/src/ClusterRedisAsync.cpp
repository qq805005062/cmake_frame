
#include <string.h>

#include "Atomic.h"
#include "Incommon.h"
#include "Thread.h"
#include "ThreadPool.h"
#include "OrderInfo.h"
#include "RedisClient.h"
#include "LibeventIo.h"
#include "../ClusterRedisAsync.h"

namespace CLUSTER_REDIS_ASYNC
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//global variable define
static std::unique_ptr<common::ThreadPool> eventIoPoolPtr(nullptr);
static std::unique_ptr<common::ThreadPool> callBackPoolPtr(nullptr);
static std::vector<common::LibeventIoPtr> libeventIoPtrVect;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//init paramter
static std::string redisSvrInfo;
static size_t inRedisSvrInfoIndex = 0;
static common::AtomicUInt32 initIoIndex;
//初始化接口传入的redis服务端信息
static REDIS_ASYNC_CLIENT::RedisSvrInfoPtrVect inRedisClusterSvrInfo;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//集群信息的vector
static REDIS_ASYNC_CLIENT::RedisClusterNodePtrVect redisClusterInfo;
//单点主从
static REDIS_ASYNC_CLIENT::RedisMasterSlaveNodePtr redisMasterSlave(nullptr);
//单点redis信息
static REDIS_ASYNC_CLIENT::RedisSvrCliPtr redisSingleInfo(nullptr);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ClusterRedisAsync::ClusterRedisAsync()
    :connNum_(0)
    ,callBackNum_(0)
    ,ioThreadNum_(0)
    ,state_(REDIS_ASYNC_INIT_STATE)
    ,keepSecond_(0)
    ,connOutSecond_(0)
    ,isExit_(0)
    ,initCb_(nullptr)
    ,exceCb_(nullptr)
{
    PDEBUG("ClusterRedisAsync init");
}

ClusterRedisAsync::~ClusterRedisAsync()
{
    PERROR("~ClusterRedisAsync exit");
}

void ClusterRedisAsync::redisAsyncExit()
{
    isExit_ = 1;
}

void ClusterRedisAsync::redisAsyncSetCallback(const InitCallback& initcb, const ExceptionCallBack& excecb)
{
    initCb_ = initcb;
    exceCb_ = excecb;
}

int ClusterRedisAsync::redisAsyncInit(int threadNum, int callbackNum, const std::string& ipPortInfo, int connOutSecond, int keepSecond, int connNum)
{
    if(connNum_ && ioThreadNum_ && callBackNum_)
    {
        PERROR("ClusterRedisAsync had been init already");
        return 0;
    }

    if(ipPortInfo.empty())
    {
        PERROR("redis svr info empty");
        return -1;
    }

    eventIoPoolPtr.reset(new common::ThreadPool("eventIo"));
    if(eventIoPoolPtr == nullptr)
    {
        PERROR("redisAsyncInit IO thread pool new error");
        return -2;
    }
    eventIoPoolPtr->start(threadNum);

    callBackPoolPtr.reset(new common::ThreadPool("callbk"));
    if(callBackPoolPtr == nullptr)
    {
        PERROR("redisAsyncInit CALLBACK thread pool new error");
        return -2;
    }
    callBackPoolPtr->start(callbackNum);

    redisSvrInfo.assign(ipPortInfo);
    connNum_ = connNum;
    ioThreadNum_ = threadNum;
    callBackNum_ = callbackNum;
    keepSecond_ = keepSecond;
    connOutSecond_ = connOutSecond;
    
    for(int i = 0; i < threadNum; i++)
    {
        common::LibeventIoPtr tmpEventIo(new common::LibeventIo());
        if(tmpEventIo == nullptr)
        {
            return -2;
        }
        libeventIoPtrVect.push_back(tmpEventIo);
    }

    for(int i = 0; i < callbackNum; i++)
    {
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::ClusterRedisAsync::cmdReplyCallPool, this));
    }

    eventIoPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::ClusterRedisAsync::redisAsyncConnect, this));
    return 0;
}

void ClusterRedisAsync::redisSvrOnConnect(int state, const std::string& ipaddr, int port)
{
    if(state_ == REDIS_ASYNC_INIT_STATE)
    {
        switch(state)
        {
            case CONNECT_REDISVR_SUCCESS:
                {
                    break;
                }
            case CONNECT_REDISVR_RESET:
                {
                    break;
                }
            case REDISVR_CONNECT_DISCONN:
                {
                    break;
                }
            default:
                {
                    break;
                }
        }
    }else{
        switch(state)
        {
            case CONNECT_REDISVR_SUCCESS:
                {
                    break;
                }
            case CONNECT_REDISVR_RESET:
                {
                    break;
                }
            case REDISVR_CONNECT_DISCONN:
                {
                    break;
                }
            default:
                {
                    break;
                }
        }
    }
}

void ClusterRedisAsync::cmdReplyCallPool()
{
    PDEBUG("callback thread pool entry");
    while(1)
    {
        common::OrderNodePtr node = common::CmdResultQueue::instance().takeCmdResult();
        if(node == nullptr && isExit_)
        {
            break;
        }
        if(node && node->resultCb_)
        {
            node->resultCb_(node->cmdRet_, node->cmdPri_, node->cmdResult_);
        }
    }
    PDEBUG("callback thread pool out");
}

void ClusterRedisAsync::redisAsyncConnect()
{
    size_t len = 0;
    int errirRet = 0;
    const char *pBegin = redisSvrInfo.c_str(), *pComma = NULL, *pColon = NULL;

    for(int i = 0; i < ioThreadNum_; i++)
    {
        eventIoPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::ClusterRedisAsync::libeventIoThread, this, i));
    }
    
    do{
        pComma = utilFristConstchar(pBegin, ',');

        pColon = utilFristConstchar(pBegin, ':');
        if(pColon == NULL)
        {
            errirRet = INIT_PARAMETER_ERROR;//ip port格式不正确
            break;
        }

        len = pColon - pBegin;
        if(len > MAX_IP_ADDR_LEN)
        {
            errirRet = INIT_PARAMETER_ERROR;//ip port格式不正确
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
            errirRet = INIT_PARAMETER_ERROR;//ip port格式不正确
            break;
        }
        int port = atoi(pColon);

        REDIS_ASYNC_CLIENT::RedisSvrInfoPtr tmpSvrInfo(new REDIS_ASYNC_CLIENT::RedisSvrInfo(port, ipAddr));
        if(tmpSvrInfo == nullptr)
        {
            errirRet = INIT_SYSTEM_ERROR;//内存分配失败
            char initMsgBuf[1024] = {0};
            sprintf(initMsgBuf, "%s::%d REDIS_ASYNC_CLIENT::RedisSvrInfo", ipAddr.c_str(), port);
            asyncInitCallBack(errirRet, initMsgBuf);
            return;
        }
        inRedisClusterSvrInfo.push_back(tmpSvrInfo);
        if(pComma)
        {
            pBegin = pComma + 1;
        }
    }while(pComma);

    if(errirRet)
    {
        REDIS_ASYNC_CLIENT::RedisSvrInfoPtrVect ().swap(inRedisClusterSvrInfo);
        std::string tmpSvrInfoStr = redisSvrInfo;
        redisSvrInfo.assign("");
        asyncInitCallBack(errirRet, tmpSvrInfoStr);
        return;
    }

    size_t tmpIoIndex = initIoIndex.incrementAndGet();
    tmpIoIndex = tmpIoIndex % ioThreadNum_;
    REDIS_ASYNC_CLIENT::RedisClientPtr redisClientP(new REDIS_ASYNC_CLIENT::RedisClient(tmpIoIndex, inRedisClusterSvrInfo[inRedisSvrInfoIndex]));
    if(redisClientP == nullptr)
    {
        errirRet = INIT_SYSTEM_ERROR;
        char initMsgBuf[1024] = {0};
        sprintf(initMsgBuf, "%s::%d REDIS_ASYNC_CLIENT::RedisClient", inRedisClusterSvrInfo[inRedisSvrInfoIndex]->ipAddr_.c_str(), inRedisClusterSvrInfo[inRedisSvrInfoIndex]->port_);
        asyncInitCallBack(errirRet, initMsgBuf);
        return;
    }

    redisSingleInfo.reset(new REDIS_ASYNC_CLIENT::RedisSvrCli(inRedisClusterSvrInfo[inRedisSvrInfoIndex], redisClientP));
    if(redisSingleInfo == nullptr)
    {
        errirRet = INIT_SYSTEM_ERROR;
        char initMsgBuf[1024] = {0};
        sprintf(initMsgBuf, "%s::%d REDIS_ASYNC_CLIENT::RedisSvrCli", inRedisClusterSvrInfo[inRedisSvrInfoIndex]->ipAddr_.c_str(), inRedisClusterSvrInfo[inRedisSvrInfoIndex]->port_);
        asyncInitCallBack(errirRet, initMsgBuf);
        return;
    }

    common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(redisClientP));
    if(node == nullptr)
    {
        errirRet = INIT_SYSTEM_ERROR;
        char initMsgBuf[1024] = {0};
        sprintf(initMsgBuf, "%s::%d common::OrderNode", inRedisClusterSvrInfo[inRedisSvrInfoIndex]->ipAddr_.c_str(), inRedisClusterSvrInfo[inRedisSvrInfoIndex]->port_);
        asyncInitCallBack(errirRet, initMsgBuf);
        return;
    }
    libeventIoPtrVect[tmpIoIndex]->libeventIoOrder(node);
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

#if 0
void ClusterRedisAsync::initConnectException(int exceCode, std::string& exceMsg)
{
    size_t tmpInitIndex = inRedisSvrInfoIndex;
    inRedisSvrInfoIndex++;
    if(inRedisClusterSvrInfo.size() > inRedisSvrInfoIndex)
    {
        asyncExceCallBack(exceCode, exceMsg);

        size_t tmpIoIndex = initIoIndex.incrementAndGet();
        tmpIoIndex = tmpIoIndex % ioThreadNum_;
        REDIS_ASYNC_CLIENT::RedisClientPtr redisClientP(new REDIS_ASYNC_CLIENT::RedisClient(tmpIoIndex, inRedisClusterSvrInfo[inRedisSvrInfoIndex]->ipAddr_, inRedisClusterSvrInfo[inRedisSvrInfoIndex]->port_));
        if(redisClientP == nullptr)
        {
            if(exceCb_)
            {
                char exceMsgBuf[1024] = {0};
                sprintf(exceMsgBuf, "%s::%d RedisClientPtr malloc nullptr", inRedisClusterSvrInfo[inRedisSvrInfoIndex]->ipAddr_, inRedisClusterSvrInfo[inRedisSvrInfoIndex]->port_);
                exceCb_(EXCE_RUNING_SYSTEM_ERROR, exceMsgBuf);
            }
            initConnectException();
        }

        redisSingleInfo.reset(new REDIS_ASYNC_CLIENT::RedisSvrCli(inRedisClusterSvrInfo[inRedisSvrInfoIndex], redisClientP));
        if(redisSingleInfo == nullptr)
        {
            if(exceCb_)
            {
                char exceMsgBuf[1024] = {0};
                sprintf(exceMsgBuf, "%s::%d RedisSvrCli malloc nullptr", inRedisClusterSvrInfo[inRedisSvrInfoIndex]->ipAddr_, inRedisClusterSvrInfo[inRedisSvrInfoIndex]->port_);
                exceCb_(EXCE_RUNING_SYSTEM_ERROR, exceMsgBuf);
            }
            initConnectException();
        }

        common::OrderNodePtr node(new common::OrderNode(redisClientP));
        if(node == nullptr)
        {
            if(exceCb_)
            {
                char exceMsgBuf[1024] = {0};
                sprintf(exceMsgBuf, "%s::%d OrderNode malloc nullptr", inRedisClusterSvrInfo[inRedisSvrInfoIndex]->ipAddr_, inRedisClusterSvrInfo[inRedisSvrInfoIndex]->port_);
                exceCb_(EXCE_RUNING_SYSTEM_ERROR, exceMsgBuf);
            }
            initConnectException();
        }
        libeventIoPtrVect[tmpIoIndex]->libeventIoOrder(node);
    }else{
        asyncInitCallBack(INIT_CONNECT_FAILED, redisSvrInfo);
    }
}
#endif
void ClusterRedisAsync::asyncInitCallBack(int initCode, const std::string& initMsg)
{
    if(initCb_)
    {
        switch(initCode)
        {
            case INIT_SUCCESS_CODE:
                {
                    initCb_(initCode, "success");
                    break;
                }
            case INIT_PARAMETER_ERROR:
                {
                    char initMsgBuf[1024] = {0};
                    sprintf(initMsgBuf, "%s parameter in error", initMsg.c_str());
                    initCb_(initCode, initMsgBuf);
                    break;
                }
            case INIT_SYSTEM_ERROR:
                {
                    char initMsgBuf[1024] = {0};
                    sprintf(initMsgBuf, "%s malloc nullptr", initMsg.c_str());
                    initCb_(initCode, initMsgBuf);
                    break;
                }
            case INIT_CONNECT_FAILED:
                {
                    char initMsgBuf[1024] = {0};
                    sprintf(initMsgBuf, "%s connect failed", initMsg.c_str());
                    initCb_(initCode, initMsgBuf);
                    break;
                }
            default:
                {
                    char initMsgBuf[1024] = {0};
                    sprintf(initMsgBuf, "%s unkown error", initMsg.c_str());
                    initCb_(initCode, initMsgBuf);
                    break;
                }
        }
    }
}

void ClusterRedisAsync::asyncExceCallBack(int exceCode, const std::string& exceMsg)
{
    if(exceCb_)
    {
        switch(exceCode)
        {
            case EXCE_INIT_CONN_FAILED:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "%s init redis svr connect failed", exceMsg.c_str());
                    exceCb_(exceCode, exceMsgBuf);
                    break;
                }
            case EXCE_RUNING_CONN_FAILED:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "%s redis svr connect failed", exceMsg.c_str());
                    exceCb_(exceCode, exceMsgBuf);
                    break;
                }
            case EXCE_RUNING_DISCONN:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "%s redis svr disconnect", exceMsg.c_str());
                    exceCb_(exceCode, exceMsgBuf);
                    break;
                }
            case EXCE_SYSTEM_ERROR:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "%s system malloc nullptr", exceMsg.c_str());
                    exceCb_(exceCode, exceMsgBuf);
                    break;
                }
            default:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "%s unkown exception", exceMsg.c_str());
                    exceCb_(exceCode, exceMsgBuf);
                    break;
                }
        }
    }
}

int ClusterRedisAsync::set(const std::string& key, const std::string& value, int outSecond, const CmdResultCallback& cb, void *priv)
{
    return 0;
}

int ClusterRedisAsync::redisAsyncCommand(const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const char *format, ...)
{
    return 0;
}

int ClusterRedisAsync::redisAsyncCommand(const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const std::string& cmdStr)
{
    return 0;
}


}

