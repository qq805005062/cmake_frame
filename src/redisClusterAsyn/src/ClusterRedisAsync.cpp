
#include <string.h>

#include "Atomic.h"
#include "Incommon.h"
#include "Thread.h"
#include "ThreadPool.h"
#include "OrderInfo.h"
#include "LibeventIo.h"
#include "RedisClientMgr.h"
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
static common::AtomicUInt32 initIoIndex;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static REDIS_ASYNC_CLIENT::VectRedisClientMgrPtr vectRedisCliMgr;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ClusterRedisAsync::ClusterRedisAsync()
    :callBackNum_(0)
    ,ioThreadNum_(0)
    ,keepSecond_(0)
    ,connOutSecond_(0)
    ,isExit_(0)
    ,initMutex_()
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

int ClusterRedisAsync::redisAsyncInit(int threadNum, int callbackNum, int connOutSecond, int keepSecond, const InitCallback& initcb, const ExceptionCallBack& excecb)
{
    std::lock_guard<std::mutex> lock(initMutex_);
    if(callBackNum_ && ioThreadNum_ && keepSecond_ && connOutSecond_)
    {
        PERROR("set already");
        return INIT_SUCCESS_CODE;
    }

    if(threadNum < 1 || callbackNum < 1 || connOutSecond < 1 || keepSecond < 1)
    {
        PERROR("threadNum %d callbackNum %d connOutSecond %d keepSecond %d", threadNum, callbackNum, connOutSecond, keepSecond);
        return INIT_PARAMETER_ERROR;
    }

    eventIoPoolPtr.reset(new common::ThreadPool("eventIo"));
    if(eventIoPoolPtr == nullptr)
    {
        PERROR("redisAsyncInit IO thread pool new error");
        return INIT_SYSTEM_ERROR;
    }
    eventIoPoolPtr->start(threadNum);

    callBackPoolPtr.reset(new common::ThreadPool("callbk"));
    if(callBackPoolPtr == nullptr)
    {
        PERROR("redisAsyncInit CALLBACK thread pool new error");
        return INIT_SYSTEM_ERROR;
    }
    callBackPoolPtr->start(callbackNum);

    ioThreadNum_ = threadNum;
    callBackNum_ = callbackNum;
    connOutSecond_ = connOutSecond;
    keepSecond_ = keepSecond;
    initCb_ = initcb;
    exceCb_ = excecb;

    for(int i = 0; i < threadNum; i++)
    {
        common::LibeventIoPtr tmpEventIo(new common::LibeventIo());
        if(tmpEventIo == nullptr)
        {
            return INIT_SYSTEM_ERROR;
        }
        libeventIoPtrVect.push_back(tmpEventIo);
    }

    for(int i = 0; i < ioThreadNum_; i++)
    {
        eventIoPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::ClusterRedisAsync::libeventIoThread, this, i));
    }

    for(int i = 0; i < callbackNum; i++)
    {
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::ClusterRedisAsync::cmdReplyCallPool, this));
    }

    return INIT_SUCCESS_CODE;
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

int ClusterRedisAsync::addSigleRedisInfo(const std::string& ipPortInfo)
{
    int asyFd = 0;
    std::lock_guard<std::mutex> lock(initMutex_);
    if((callBackNum_ == 0) || (ioThreadNum_ == 0) || (keepSecond_ == 0) || (connOutSecond_ == 0))
    {
        PERROR("threadNum %d callbackNum %d connOutSecond %d keepSecond %d", ioThreadNum_, callBackNum_, connOutSecond_, keepSecond_);
        return INIT_NO_INIT_ERROR;
    }

    REDIS_ASYNC_CLIENT::RedisClientMgrPtr client(new REDIS_ASYNC_CLIENT::RedisClientMgr());
    if(client == nullptr)
    {
        return INIT_SYSTEM_ERROR;
    }

    client->inSvrInfoStr_.assign(ipPortInfo);
    client->cliState_ = REDIS_SINGLE_SERVER;
    client->mgrFd_ = static_cast<int>(vectRedisCliMgr.size());
    asyFd = static_cast<int>(vectRedisCliMgr.size());
    vectRedisCliMgr.push_back(client);

    int ret = redisInitConnect();
    if(ret < 0)
    {
        return ret;
    }
    return asyFd;
}

int ClusterRedisAsync::addMasterSlaveInfo(const std::string& ipPortInfo)
{
    int asyFd = 0;
    std::lock_guard<std::mutex> lock(initMutex_);
    if((callBackNum_ == 0) || (ioThreadNum_ == 0) || (keepSecond_ == 0) || (connOutSecond_ == 0))
    {
        PERROR("threadNum %d callbackNum %d connOutSecond %d keepSecond %d", ioThreadNum_, callBackNum_, connOutSecond_, keepSecond_);
        return INIT_NO_INIT_ERROR;
    }

    REDIS_ASYNC_CLIENT::RedisClientMgrPtr client(new REDIS_ASYNC_CLIENT::RedisClientMgr());
    if(client == nullptr)
    {
        return INIT_SYSTEM_ERROR;
    }

    client->inSvrInfoStr_.assign(ipPortInfo);
    client->cliState_ = REDIS_MASTER_SLAVE_SERVER;
    client->mgrFd_ = static_cast<int>(vectRedisCliMgr.size());
    asyFd = static_cast<int>(vectRedisCliMgr.size());
    vectRedisCliMgr.push_back(client);

    int ret = redisInitConnect();
    if(ret < 0)
    {
        return ret;
    }
    return asyFd;
}

int ClusterRedisAsync::addClusterInfo(const std::string& ipPortInfo)
{
    int asyFd = 0;
    std::lock_guard<std::mutex> lock(initMutex_);
    if((callBackNum_ == 0) || (ioThreadNum_ == 0) || (keepSecond_ == 0) || (connOutSecond_ == 0))
    {
        PERROR("threadNum %d callbackNum %d connOutSecond %d keepSecond %d", ioThreadNum_, callBackNum_, connOutSecond_, keepSecond_);
        return INIT_NO_INIT_ERROR;
    }

    REDIS_ASYNC_CLIENT::RedisClientMgrPtr client(new REDIS_ASYNC_CLIENT::RedisClientMgr());
    if(client == nullptr)
    {
        return INIT_SYSTEM_ERROR;
    }

    client->inSvrInfoStr_.assign(ipPortInfo);
    client->cliState_ = REDIS_MASTER_SLAVE_SERVER;
    client->mgrFd_ = static_cast<int>(vectRedisCliMgr.size());
    asyFd = static_cast<int>(vectRedisCliMgr.size());
    vectRedisCliMgr.push_back(client);

    int ret = redisInitConnect();
    if(ret < 0)
    {
        return ret;
    }
    return asyFd;
}

int ClusterRedisAsync::redisInitConnect()
{
    size_t len = vectRedisCliMgr.size() - 1;
    //这个取最后一个新插入的节点初始化连接。有点奇怪。主要为了解决传参的问题，如果传参倒是好解决。但是为了尽可能少暴露模块内的头文件及定义出去
    REDIS_ASYNC_CLIENT::RedisClientMgrPtr cli = vectRedisCliMgr[len];
    const char *pBegin = cli->inSvrInfoStr_.c_str(), *pComma = NULL, *pColon = NULL;

    do{
        pComma = utilFristConstchar(pBegin, ',');

        pColon = utilFristConstchar(pBegin, ':');
        if(pColon == NULL)
        {
            return INIT_PARAMETER_ERROR;//ip port格式不正确
        }

        len = pColon - pBegin;
        if(len > MAX_IP_ADDR_LEN)
        {
            return INIT_PARAMETER_ERROR;//ip port格式不正确
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
            return INIT_PARAMETER_ERROR;//ip port格式不正确
        }
        int port = atoi(pColon);

        REDIS_ASYNC_CLIENT::RedisSvrInfoPtr tmpSvrInfo(new REDIS_ASYNC_CLIENT::RedisSvrInfo(port, ipAddr));
        if(tmpSvrInfo == nullptr)
        {
            return INIT_SYSTEM_ERROR;//内存分配失败
        }
        cli->initSvrInfo_.push_back(tmpSvrInfo);
        if(pComma)
        {
            pBegin = pComma + 1;
        }
    }while(pComma);

    size_t tmpIoIndex = initIoIndex.incrementAndGet();
    tmpIoIndex = tmpIoIndex % ioThreadNum_;
    REDIS_ASYNC_CLIENT::RedisClientPtr tmpRedisCli(new REDIS_ASYNC_CLIENT::RedisClient(tmpIoIndex, cli->mgrFd_, cli->initSvrInfo_[cli->initSvrIndex_], keepSecond_, connOutSecond_));
    if(tmpRedisCli == nullptr)
    {
        #if 0
        errirRet = INIT_SYSTEM_ERROR;
        char initMsgBuf[1024] = {0};
        sprintf(initMsgBuf, "%s::%d REDIS_ASYNC_CLIENT::RedisClient", inRedisClusterSvrInfo[inRedisSvrInfoIndex]->ipAddr_.c_str(), inRedisClusterSvrInfo[inRedisSvrInfoIndex]->port_);
        asyncInitCallBack(errirRet, initMsgBuf);
        #endif
        return INIT_SYSTEM_ERROR;
    }

    REDIS_ASYNC_CLIENT::RedisSvrCliPtr tmpRedisSvrCli(new REDIS_ASYNC_CLIENT::RedisSvrCli(cli->initSvrInfo_[cli->initSvrIndex_], tmpRedisCli));
    if(tmpRedisSvrCli == nullptr)
    {
        return INIT_SYSTEM_ERROR;
    }
    cli->nodeCli_ = tmpRedisSvrCli;

    common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(tmpRedisCli));
    if(node == nullptr)
    {
        return INIT_SYSTEM_ERROR;
    }
    
    libeventIoPtrVect[tmpIoIndex]->libeventIoOrder(node);
    return INIT_SUCCESS_CODE;
}

void ClusterRedisAsync::redisSvrOnConnect(int state, const std::string& ipaddr, int port)
{
    if(true)
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
void ClusterRedisAsync::asyncInitCallBack(int asyFd, int initCode, const std::string& initMsg)
{
    if(initCb_)
    {
        switch(initCode)
        {
            case INIT_SUCCESS_CODE:
                {
                    initCb_(asyFd, initCode, "success");
                    break;
                }
            case INIT_PARAMETER_ERROR:
                {
                    char initMsgBuf[1024] = {0};
                    sprintf(initMsgBuf, "%s parameter in error", initMsg.c_str());
                    initCb_(asyFd, initCode, initMsgBuf);
                    break;
                }
            case INIT_SYSTEM_ERROR:
                {
                    char initMsgBuf[1024] = {0};
                    sprintf(initMsgBuf, "%s malloc nullptr", initMsg.c_str());
                    initCb_(asyFd, initCode, initMsgBuf);
                    break;
                }
            case INIT_CONNECT_FAILED:
                {
                    char initMsgBuf[1024] = {0};
                    sprintf(initMsgBuf, "%s connect failed", initMsg.c_str());
                    initCb_(asyFd, initCode, initMsgBuf);
                    break;
                }
            default:
                {
                    char initMsgBuf[1024] = {0};
                    sprintf(initMsgBuf, "%s unkown error", initMsg.c_str());
                    initCb_(asyFd, initCode, initMsgBuf);
                    break;
                }
        }
    }
}

void ClusterRedisAsync::asyncExceCallBack(int asyFd, int exceCode, const std::string& exceMsg)
{
    if(exceCb_)
    {
        switch(exceCode)
        {
            case EXCE_INIT_CONN_FAILED:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "%s init redis svr connect failed", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
            case EXCE_RUNING_CONN_FAILED:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "%s redis svr connect failed", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
            case EXCE_RUNING_DISCONN:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "%s redis svr disconnect", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
            case EXCE_SYSTEM_ERROR:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "%s system malloc nullptr", exceMsg.c_str());
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

