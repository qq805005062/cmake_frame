
#include <string.h>

#include "Atomic.h"
#include "Incommon.h"
#include "Thread.h"
#include "ThreadPool.h"
#include "OrderInfo.h"
#include "LibeventIo.h"
#include "RedisClientMgr.h"
#include "../RedisAsync.h"

namespace CLUSTER_REDIS_ASYNC
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//global variable define
static std::unique_ptr<common::ThreadPool> eventIoPoolPtr(nullptr);
static std::unique_ptr<common::ThreadPool> callBackPoolPtr(nullptr);
static std::unique_ptr<common::Thread> timerThreadPtr(nullptr);

static std::vector<common::LibeventIoPtr> libeventIoPtrVect;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//init paramter
static common::AtomicUInt32 initIoIndex;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static REDIS_ASYNC_CLIENT::VectRedisClientMgrPtr vectRedisCliMgr;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RedisAsync::RedisAsync()
    :callBackNum_(0)
    ,ioThreadNum_(0)
    ,keepSecond_(0)
    ,connOutSecond_(0)
    ,isExit_(0)
    ,nowSecond_(0)
    ,initMutex_()
    ,stateCb_(nullptr)
    ,exceCb_(nullptr)
{
    PDEBUG("RedisAsync init");
}

RedisAsync::~RedisAsync()
{
    PERROR("~RedisAsync exit");
}

void RedisAsync::redisAsyncExit()
{
    isExit_ = 1;
    
    //TODO
    if(callBackPoolPtr)
    {
        callBackPoolPtr->stop();
    }

    if(timerThreadPtr)
    {
        timerThreadPtr->join();
    }

    if(eventIoPoolPtr)
    {
        eventIoPoolPtr->stop();
    }
}

int RedisAsync::redisAsyncInit(int threadNum, int callbackNum, int connOutSecond, int keepSecond, const StateChangeCb& statecb, const ExceptionCallBack& excecb)
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

    timerThreadPtr.reset(new common::Thread(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::timerThreadRun, this), "timer"));
    if(timerThreadPtr == nullptr)
    {
        PERROR("redisAsyncInit TIMER thread new error");
        return INIT_SYSTEM_ERROR;
    }
    timerThreadPtr->start();

    ioThreadNum_ = threadNum;
    callBackNum_ = callbackNum;
    connOutSecond_ = connOutSecond;
    keepSecond_ = keepSecond;
    stateCb_ = statecb;
    exceCb_ = excecb;
    nowSecond_ = secondSinceEpoch();

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
        eventIoPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::libeventIoThread, this, i));
    }

    return INIT_SUCCESS_CODE;
}

void RedisAsync::timerThreadRun()
{
    while(1)
    {
        if(isExit_)
        {
            break;
        
}
        sleep(1);
        if(isExit_)
        {
            break;
        
}

        nowSecond_ = secondSinceEpoch();
        size_t subIndex = 0;
        for(int i = 0; i < ioThreadNum_; i++)
        {
            if(libeventIoPtrVect[subIndex])
            {
                libeventIoPtrVect[subIndex]->libeventIoWakeup(nowSecond_);
            }
            subIndex++;
        }
    }
}

void RedisAsync::libeventIoThread(int index)
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

void RedisAsync::cmdReplyCallPool()
{
    PDEBUG("cb thread poll callback");
    common::OrderNodePtr node = common::CmdResultQueue::instance().takeCmdResult();
    if(node == nullptr)
    {
        PERROR("cb thread poll get node nullptr");
        return;
    }
    if(node && node->resultCb_)
    {
        node->resultCb_(node->cmdRet_, node->cmdPri_, node->cmdResult_);
    }
}

void RedisAsync::asyncCmdResultCallBack()
{
    PDEBUG("IO Asynchronous ready cb");
    callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::cmdReplyCallPool, this));
}

void RedisAsync::clusterInitCallBack(int ret, void* priv, const StdVectorStringPtr& resultMsg)
{
    PDEBUG("ret %d priv %p", ret , priv);
    for(StdVectorStringPtr::const_iterator iter = resultMsg.begin(); iter != resultMsg.end(); iter++)
    {
        PDEBUG("\n%s", (*iter)->c_str());
    }
    return;
}

void RedisAsync::masterSalveInitCb(int ret, void* priv, const StdVectorStringPtr& resultMsg)
{
    PDEBUG("ret %d priv %p", ret , priv);
    for(StdVectorStringPtr::const_iterator iter = resultMsg.begin(); iter != resultMsg.end(); iter++)
    {
        PDEBUG("%s", (*iter)->c_str());
    }
    return;
}

int RedisAsync::addSigleRedisInfo(const std::string& ipPortInfo)
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
    client->svrType_ = REDIS_SINGLE_SERVER;
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

int RedisAsync::addMasterSlaveInfo(const std::string& ipPortInfo)
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
    client->svrType_ = REDIS_MASTER_SLAVE_SERVER;
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

int RedisAsync::addClusterInfo(const std::string& ipPortInfo)
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
    client->svrType_ = REDIS_CLUSTER_SERVER;
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

int RedisAsync::redisInitConnect()
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
    
    libeventIoPtrVect[tmpIoIndex]->libeventIoOrder(node, nowSecond_);
    return INIT_SUCCESS_CODE;
}

/*
 * [redisSvrOnConnect] 这一个方法挺复杂的，当有redis服务连接成功或者断开连接，会回调此函数。要区分各种情况。所以这个函数看起来会比较冗余复杂，注意区分各种情况即可
 * @author xiaoxiao 2019-05-14
 * @param
 * @param
 * @param
 *
 * @return
 */
void RedisAsync::redisSvrOnConnect(size_t asyFd, int state, const std::string& ipaddr, int port)
{
    if(vectRedisCliMgr.size() <= asyFd)
    {
        PERROR("vectRedisCliMgr.size() %ld asyFd %ld", vectRedisCliMgr.size(), asyFd);
        return;
    }
    PDEBUG("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
    switch(vectRedisCliMgr[asyFd]->svrType_)
    {
        //vectRedisCliMgr[asyFd]->svrType_ 服务类型
        case REDIS_UNKNOWN_SERVER:
            {
                PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                //TODO
                return;
            }
        //vectRedisCliMgr[asyFd]->svrType_ 服务类型
        case REDIS_SINGLE_SERVER:
            {
                switch(vectRedisCliMgr[asyFd]->initState_)
                {
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    case REDIS_SVR_INIT_STATE:
                        {
                            switch(state)
                            {
                                //function state 连接状态
                                case CONNECT_REDISVR_SUCCESS:
                                    {
                                        vectRedisCliMgr[asyFd]->initState_ = REDIS_SVR_RUNING_STATE;
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, asyFd, REDIS_SVR_RUNING_STATE, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(vectRedisCliMgr[asyFd]->nodeCli_->redisClient_));
                                        if(node == nullptr)
                                        {
                                            char exceMsgBuf[1024] = {0};
                                            sprintf(exceMsgBuf, "%s::%d", ipaddr.c_str(), port);
                                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_RECONNECT_NEWORDER_NULL, exceMsgBuf));
                                            //TODO
                                            return;
                                        }
                                        libeventIoPtrVect[vectRedisCliMgr[asyFd]->nodeCli_->redisClient_->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                                //function state 连接状态
                                default:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                            }
                            return;
                        }
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    case REDIS_SVR_RUNING_STATE:
                        {
                            switch(state)
                            {
                                //function state 连接状态
                                case CONNECT_REDISVR_SUCCESS:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        vectRedisCliMgr[asyFd]->initState_ = REDIS_SVR_ERROR_STATE;
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, asyFd, REDIS_SVR_ERROR_STATE, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
                                        common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(vectRedisCliMgr[asyFd]->nodeCli_->redisClient_));
                                        if(node == nullptr)
                                        {
                                            char exceMsgBuf[1024] = {0};
                                            sprintf(exceMsgBuf, "%s::%d", ipaddr.c_str(), port);
                                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_RECONNECT_NEWORDER_NULL, exceMsgBuf));
                                            //TODO
                                            return;
                                        }
                                        libeventIoPtrVect[vectRedisCliMgr[asyFd]->nodeCli_->redisClient_->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
                                        return;
                                    }
                                //function state 连接状态
                                default:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                            }
                            return;
                        }
                    case REDIS_EXCEPTION_STATE:
                        {
                            PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                            return;
                        }
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    case REDIS_SVR_ERROR_STATE:
                        {
                            switch(state)
                            {
                                //function state 连接状态
                                case CONNECT_REDISVR_SUCCESS:
                                    {
                                        vectRedisCliMgr[asyFd]->initState_ = REDIS_SVR_RUNING_STATE;
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, asyFd, REDIS_SVR_RUNING_STATE, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        //TODO 重连时间要拉长
                                        common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(vectRedisCliMgr[asyFd]->nodeCli_->redisClient_));
                                        if(node == nullptr)
                                        {
                                            char exceMsgBuf[1024] = {0};
                                            sprintf(exceMsgBuf, "%s::%d", ipaddr.c_str(), port);
                                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_RECONNECT_NEWORDER_NULL, exceMsgBuf));
                                            //TODO
                                            return;
                                        }
                                        libeventIoPtrVect[vectRedisCliMgr[asyFd]->nodeCli_->redisClient_->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                                //function state 连接状态
                                default:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                            }
                            return;
                        }
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    default:
                        {
                            PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                            return;
                        }
                }
                return;
            }
        //vectRedisCliMgr[asyFd]->svrType_ 服务类型
        case REDIS_MASTER_SLAVE_SERVER:
            {
                switch(vectRedisCliMgr[asyFd]->initState_)
                {
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    case REDIS_SVR_INIT_STATE:
                        {
                            switch(state)
                            {
                                //function state 连接状态
                                case CONNECT_REDISVR_SUCCESS:
                                    {
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        return;
                                    }
                                //function state 连接状态
                                default:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                            }
                            return;
                        }
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    case REDIS_SVR_RUNING_STATE:
                        {
                            return;
                        }
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    case REDIS_EXCEPTION_STATE:
                        {
                            return;
                        }
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    case REDIS_SVR_ERROR_STATE:
                        {
                            return;
                        }
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    default:
                        {
                            PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                            return;
                        }
                }
                return;
            }
        //vectRedisCliMgr[asyFd]->svrType_ 服务类型
        case REDIS_CLUSTER_SERVER:
            {
                switch(vectRedisCliMgr[asyFd]->initState_)
                {
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    case REDIS_SVR_INIT_STATE:
                        {
                            switch(state)
                            {
                                //function state 连接状态
                                case CONNECT_REDISVR_SUCCESS:
                                    {
                                        PDEBUG("CLUSTER NODES");
                                        std::string clusterCmd = "CLUSTER NODES";
                                        common::OrderNodePtr cmdnode(new common::OrderNode(clusterCmd, DEFAULT_CMD_OUTSECOND, std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::clusterInitCallBack, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3)));
                                        if(cmdnode == nullptr)
                                        {
                                            //TODO
                                            return;
                                        }
                                        common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(vectRedisCliMgr[asyFd]->nodeCli_->redisClient_, cmdnode, (nowSecond_ + DEFAULT_CMD_OUTSECOND)));
                                        if(node == nullptr)
                                        {
                                            //TODO
                                            return;
                                        }
                                        libeventIoPtrVect[vectRedisCliMgr[asyFd]->nodeCli_->redisClient_->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        //TODO 重连时间要拉长
                                        common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(vectRedisCliMgr[asyFd]->nodeCli_->redisClient_));
                                        if(node == nullptr)
                                        {
                                            char exceMsgBuf[1024] = {0};
                                            sprintf(exceMsgBuf, "%s::%d", ipaddr.c_str(), port);
                                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_RECONNECT_NEWORDER_NULL, exceMsgBuf));
                                            //TODO
                                            return;
                                        }
                                        libeventIoPtrVect[vectRedisCliMgr[asyFd]->nodeCli_->redisClient_->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                                //function state 连接状态
                                default:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                            }
                            return;
                        }
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    case REDIS_SVR_RUNING_STATE:
                        {
                            switch(state)
                            {
                                //function state 连接状态
                                case CONNECT_REDISVR_SUCCESS:
                                    {
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        return;
                                    }
                                //function state 连接状态
                                default:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                            }
                            return;
                        }
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    case REDIS_EXCEPTION_STATE:
                        {
                            switch(state)
                            {
                                //function state 连接状态
                                case CONNECT_REDISVR_SUCCESS:
                                    {
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        return;
                                    }
                                //function state 连接状态
                                default:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                            }
                            return;
                        }
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    case REDIS_SVR_ERROR_STATE:
                        {
                            switch(state)
                            {
                                //function state 连接状态
                                case CONNECT_REDISVR_SUCCESS:
                                    {
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        return;
                                    }
                                //function state 连接状态
                                default:
                                    {
                                        PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                                        return;
                                    }
                            }
                            return;
                        }
                    //vectRedisCliMgr[asyFd]->initState_ 服务端状态
                    default:
                        {
                            PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                            return;
                        }
                }
                break;
            }
        //vectRedisCliMgr[asyFd]->svrType_ 服务类型
        default:
            {
                PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                return;
            }
    }
    return;
}

#if 0
void RedisAsync::initConnectException(int exceCode, std::string& exceMsg)
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
        libeventIoPtrVect[tmpIoIndex]->libeventIoOrder(node, nowSecond_);
    }else{
        asyncInitCallBack(INIT_CONNECT_FAILED, redisSvrInfo);
    }
}
#endif

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
            case EXCE_RECONNECT_NEWORDER_NULL:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "reconnect new order nullptr %s", exceMsg.c_str());
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
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int RedisAsync::set(int asyFd, const std::string& key, const std::string& value, int outSecond, const CmdResultCallback& cb, void *priv)
{
    return 0;
}

int RedisAsync::redisAsyncCommand(int asyFd, const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const char *format, ...)
{
    return 0;
}

int RedisAsync::redisAsyncCommand(int asyFd, const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const std::string& cmdStr)
{
    return 0;
}


}
