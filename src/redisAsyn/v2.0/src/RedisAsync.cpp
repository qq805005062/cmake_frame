
#include <stdarg.h>
#include <string.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#include <async.h>

#include "Atomic.h"
#include "MutexLock.h"

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
static common::AtomicInt32 asyncCmdNum;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static REDIS_ASYNC_CLIENT::VectRedisClientMgrPtr vectRedisCliMgr;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RedisAsync::RedisAsync()
    :callBackNum_(0)
    ,ioThreadNum_(0)
    ,keepSecond_(0)
    ,connOutSecond_(0)
    ,isExit_(0)
    ,timerExit_(0)
    ,nowSecond_(0)
    ,initMutex_()
    ,stateCb_(nullptr)
    ,exceCb_(nullptr)
{
    PDEBUG("RedisAsync init");
}

RedisAsync::~RedisAsync()
{
    //TODO 有时间要细想一下这里的析构顺序
    std::vector<common::LibeventIoPtr> ().swap(libeventIoPtrVect);
    REDIS_ASYNC_CLIENT::VectRedisClientMgrPtr ().swap(vectRedisCliMgr);
    
    PERROR("~RedisAsync exit");
}

void RedisAsync::redisAsyncExit()
{
    isExit_ = 1;
    
    do{
        if(asyncCmdNum.get() == 0)
        {
            break;
        }
        PERROR("This still async cmd num %d", asyncCmdNum.get());
        usleep(1000);
    }while(1);
    
    if(callBackPoolPtr)
    {
        callBackPoolPtr->stop();
        callBackPoolPtr.reset();
    }
    PDEBUG("callBackPoolPtr->stop");

    timerExit_ = 1;
    if(timerThreadPtr)
    {
        timerThreadPtr->join();
        timerThreadPtr.reset();
    }
    PDEBUG("timerThreadPtr->join");

    for(int i = 0; i < ioThreadNum_; i++)
    {
        size_t index = static_cast<size_t>(i);
        if(libeventIoPtrVect[index])
        {
            libeventIoPtrVect[index]->libeventIoExit();
        }
    }

    if(eventIoPoolPtr)
    {
        eventIoPoolPtr->stop();
        eventIoPoolPtr.reset();
    }
    PDEBUG("eventIoPoolPtr->stop");
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
        if(timerExit_)
        {
            break;
        }
        sleep(1);
        if(timerExit_)
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

        do{//实现定时器功能，不是特别准的定时器。每秒钟检查有无超时的，每秒检查有无连接超时，检查有无请求超时，有无定时器方法到时间
            common::RedisCliOrderNodePtr node = common::TimerOrderDeque::instance().dealTimerOrder(nowSecond_);
            if(node)
            {
                if(node->cmdOrd_)
                {
                    node->cmdOrd_->outSecond_ = DEFAULT_CMD_OUTSECOND;
                    node->cmdOrd_->cmdOutSecond_ = nowSecond_ + DEFAULT_CMD_OUTSECOND;
                    node->outSecond_ = node->cmdOrd_->cmdOutSecond_;
                    libeventIoPtrVect[node->cli_->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
                }else{
                    node->outSecond_ = 0;
                    libeventIoPtrVect[node->cli_->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
                }
            }else{
                break;
            }
        }while(true);
    }
}

void RedisAsync::libeventIoThread(int index)
{
    int ret = 0;
    size_t subIndex = static_cast<size_t>(index);
    PDEBUG("io thread index %d entry", index);
    while(1)
    {
        if(libeventIoPtrVect[subIndex])
        {
            ret = libeventIoPtrVect[subIndex]->libeventIoReady();
            PERROR("LibeventIoPtrVect[%d]->asyncCurlReady ret %d", index, ret);
        }else{
            PERROR("LibeventTcpCli %d new io Thread object error", index);
        }
        if(isExit_)
        {
            break;
        }

        libeventIoPtrVect[subIndex].reset(new common::LibeventIo());
    }
    PDEBUG("io thread index %d out", index);

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
        asyncCmdNum.decrement();
        PTRACE("asyncCmdNum %d", asyncCmdNum.get());
        node->resultCb_(node->cmdRet_, node->cmdPri_, node->cmdResult_);
    }
}

void RedisAsync::asyncRequestCmdAdd()
{
    asyncCmdNum.increment();
    PTRACE("asyncCmdNum %d", asyncCmdNum.get());
}

void RedisAsync::asyncCmdResultCallBack()
{
    PDEBUG("IO Asynchronous ready cb");
    callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::cmdReplyCallPool, this));
}

/*
cluster_state:ok
cluster_slots_assigned:16384
cluster_slots_ok:16384
cluster_slots_pfail:0
cluster_slots_fail:0
cluster_known_nodes:6
cluster_size:3
cluster_current_epoch:7
cluster_my_epoch:7
cluster_stats_messages_ping_sent:1266118
cluster_stats_messages_pong_sent:862541
cluster_stats_messages_sent:2128659
cluster_stats_messages_ping_received:862541
cluster_stats_messages_pong_received:1266035
cluster_stats_messages_received:2128576
*/
void RedisAsync::clusterInfoCallBack(int ret, void* priv, const StdVectorStringPtr& resultMsg)
{
    PTRACE("ret %d priv %p", ret , priv);
    for(StdVectorStringPtr::const_iterator iter = resultMsg.begin(); iter != resultMsg.end(); iter++)
    {
        PTRACE("\n%s", (*iter)->c_str());
    }

    if(priv == nullptr)
    {
        PERROR("priv == nullptr");
        return;
    }
    REDIS_ASYNC_CLIENT::RedisClientMgr *pClient = static_cast<REDIS_ASYNC_CLIENT::RedisClientMgr *>(priv);
    common::SafeMutexLock clusterLock(pClient->mutex_);
    if(pClient->initState_ == REDIS_SVR_INVALID_STATE)//在这里由于内部资源已经处于释放状态，所以不需要继续往下走了
    {
        PERROR("asyFd %ld state %d", pClient->mgrFd_, pClient->initState_);
        return;
    }
    if(ret == 0)
    {
        std::vector<std::string> vLines;
        split(*resultMsg[0], "\n", vLines);

        std::string::size_type pos = vLines[0].find(':');
        if (pos == std::string::npos)
        {
            if(pClient->initState_ != REDIS_SVR_ERROR_STATE)
            {
                pClient->initState_ = REDIS_SVR_ERROR_STATE;
                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, pClient->mgrFd_, REDIS_SVR_ERROR_STATE, pClient->inSvrInfoStr_));
            }
            return;
        }

        std::string stateStr = vLines[0].substr(pos + 1);
        if(stateStr.compare("ok") == 0)
        {
            if(pClient->initState_ == REDIS_SVR_ERROR_STATE)
            {
                pClient->initState_ = REDIS_EXCEPTION_STATE;
            }
        }else{
            if(pClient->initState_ != REDIS_SVR_ERROR_STATE)
            {
                pClient->initState_ = REDIS_SVR_ERROR_STATE;
                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, pClient->mgrFd_, REDIS_SVR_ERROR_STATE, pClient->inSvrInfoStr_));
            }
        }
    }
    PDEBUG("asyFd %ld state %d ret %d", pClient->mgrFd_, pClient->initState_, ret);
}

void RedisAsync::clusterNodesCallBack(int ret, void* priv, const StdVectorStringPtr& resultMsg)
{
    PTRACE("ret %d priv %p", ret , priv);
    for(StdVectorStringPtr::const_iterator iter = resultMsg.begin(); iter != resultMsg.end(); iter++)
    {
        PTRACE("\n%s", (*iter)->c_str());
    }

    if(priv == nullptr)
    {
        PERROR("priv == nullptr");
        return;
    }
    REDIS_ASYNC_CLIENT::RedisClientMgr *pClient = static_cast<REDIS_ASYNC_CLIENT::RedisClientMgr *>(priv);
    common::SafeMutexLock clusterLock(pClient->mutex_);
    if(pClient->initState_ == REDIS_SVR_INVALID_STATE)//在这里由于内部资源已经处于释放状态，所以不需要继续往下走了
    {
        PERROR("asyFd %ld state %d", pClient->mgrFd_, pClient->initState_);
        return;
    }
    
    do{
        if(ret)
        {
            PERROR("asyFd %ld state %d ret %d", pClient->mgrFd_, pClient->initState_, ret);
            break;
        }
        bool stateOk = true;
        std::vector<std::string> vLines;
        split(*resultMsg[0], "\n", vLines);
        for (size_t i = 0; i < vLines.size(); ++i)
        {
            std::string::size_type pos = vLines[i].find("master,fail");
            if (pos != std::string::npos)
            {
                stateOk = false;
                break;
            }
        }
        if(stateOk == false)
        {
            break;
        }

        REDIS_ASYNC_CLIENT::VectRedisClientPtr tmpAddRedisClis;
        int result = analysisClusterNodes(*resultMsg[0], *pClient, initIoIndex, tmpAddRedisClis, ioThreadNum_, keepSecond_, connOutSecond_);
        switch(result)
        {
            case 0:
                {
                    for(size_t i = 0; i < tmpAddRedisClis.size(); i++)
                    {
                        common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(tmpAddRedisClis[i]));
                        if(node == nullptr)
                        {
                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, EXCE_MALLOC_NULL, pClient->inSvrInfoStr_));
                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, pClient->mgrFd_));
                            return;
                        }
                        libeventIoPtrVect[tmpAddRedisClis[i]->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
                    }
                    if(pClient->cluterCli_)
                    {
                        if((pClient->cluterCli_->clusterVectInfo_.size() == pClient->masterConn_) && (pClient->initState_ != REDIS_SVR_RUNING_STATE))
                        {
                            pClient->initState_ = REDIS_SVR_RUNING_STATE;
                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, pClient->mgrFd_, REDIS_SVR_RUNING_STATE, pClient->inSvrInfoStr_));
                        }

                        if((pClient->cluterCli_->clusterVectInfo_.size() != pClient->masterConn_) && (pClient->initState_ == REDIS_SVR_RUNING_STATE))
                        {
                            pClient->initState_ = REDIS_EXCEPTION_STATE;
                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, CLUSTER_NODES_CHANGE, pClient->inSvrInfoStr_));
                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, pClient->mgrFd_, REDIS_EXCEPTION_STATE, pClient->inSvrInfoStr_));
                        }
                    }else{
                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, pClient->mgrFd_));
                    }
                    return;
                }
            case ANALYSIS_STRING_FORMATE_ERROR:
                {
                    PERROR("asyFd %ld state %d result %d", pClient->mgrFd_, pClient->initState_, result);
                    break;
                }
            case ANALYSIS_MALLOC_NULLPTR_ERROR:
                {
                    callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, EXCE_MALLOC_NULL, pClient->inSvrInfoStr_));
                    callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, pClient->mgrFd_));
                    return;
                }
            default:
                {
                    PERROR("asyFd %ld state %d result %d", pClient->mgrFd_, pClient->initState_, result);
                    break;
                }
        }
    }while(0);

    //无论是主从还是集群，或者集群已经析构了，pClient->nodeCli_是不可以析构的，pClient->nodeCli_断连之后替换pClient->nodeCli_就在这里
    if((pClient->nodeCli_ == nullptr) || (pClient->nodeCli_->tcpCliState() == REDIS_CLIENT_STATE_INIT))
    {
        if(pClient->cluterCli_)
        {
            for(size_t i = 0; i < pClient->cluterCli_->clusterVectInfo_.size(); i++)
            {
                if(pClient->cluterCli_->clusterVectInfo_[i] && pClient->cluterCli_->clusterVectInfo_[i]->clusterNode_)
                {
                    if((pClient->cluterCli_->clusterVectInfo_[i]->clusterNode_->master_) && (pClient->cluterCli_->clusterVectInfo_[i]->clusterNode_->master_->tcpCliState() == REDIS_CLIENT_STATE_CONN))
                    {
                        pClient->nodeCli_ = pClient->cluterCli_->clusterVectInfo_[i]->clusterNode_->master_;
                        break;
                    }

                    for(size_t j = 0; j < pClient->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_.size(); j++)
                    {
                        if((pClient->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_[j]) && (pClient->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_[j]->tcpCliState() == REDIS_CLIENT_STATE_CONN))
                        {
                            pClient->nodeCli_  = pClient->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_[j];
                            break;
                        }
                    }

                    if(pClient->nodeCli_ && pClient->nodeCli_->tcpCliState() == REDIS_CLIENT_STATE_CONN)
                    {
                        break;
                    }
                }
            }
        }else{
            PERROR("asyFd %ld state %d cluterCli_ nullptr", pClient->mgrFd_, pClient->initState_);
        }
    }

    callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::clusterNodesUpdate, this, pClient->mgrFd_, true, false));//延迟检查集群信息在这里做

    std::string clusterInfo("CLUSTER INFO");
    common::OrderNodePtr cmdnode(new common::OrderNode(clusterInfo, std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::clusterInfoCallBack, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3), static_cast<void*>(pClient)));
    if(cmdnode == nullptr)
    {
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, EXCE_MALLOC_NULL, pClient->inSvrInfoStr_));
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, pClient->mgrFd_));
        return;
    }
    common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(pClient->nodeCli_, cmdnode, (nowSecond_ + DEFAULT_DELAY_CMD_SECOND)));
    if(node == nullptr)
    {
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, EXCE_MALLOC_NULL, pClient->inSvrInfoStr_));
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, pClient->mgrFd_));
        return;
    }
    common::TimerOrderDeque::instance().insertTimerOrder(node);
}

/*
9be7650002bdbfd6b099c07969c6969d120b2b6c 192.169.6.211:6790@16790 master - 0 1557940444069 5 connected 10923-16383
af805d65d9ee40af96695a213b300afd2f5e24d6 192.169.6.234:6791@16791 slave a4892e9a69d9e2858a9d0f45a915c40734b8a4aa 0 1557940443067 3 connected
1adf947915d6f3f4bc9943831734d34b7e1fc610 192.169.6.234:6790@16790 myself,slave 19b1e55b4b30bb9c48b504121f36bd253fe7d30f 0 1557940442000 1 connected
37e495750f36270d3a55a41a126f940316c54c8c 192.169.6.211:6791@16791 slave 9be7650002bdbfd6b099c07969c6969d120b2b6c 0 1557940446071 6 connected
a4892e9a69d9e2858a9d0f45a915c40734b8a4aa 192.169.6.233:6790@16790 master - 0 1557940441000 3 connected 5461-10922
19b1e55b4b30bb9c48b504121f36bd253fe7d30f 192.169.6.233:6791@16791 master - 0 1557940445070 7 connected 0-5460
*/
void RedisAsync::clusterInitCallBack(int ret, void* priv, const StdVectorStringPtr& resultMsg)
{
    PTRACE("ret %d priv %p", ret , priv);
    for(StdVectorStringPtr::const_iterator iter = resultMsg.begin(); iter != resultMsg.end(); iter++)
    {
        PTRACE("\n%s", (*iter)->c_str());
    }

    if(priv == nullptr)
    {
        PERROR("priv == nullptr");
        return;
    }
    REDIS_ASYNC_CLIENT::RedisClientMgr *pClient = static_cast<REDIS_ASYNC_CLIENT::RedisClientMgr *>(priv);
    common::SafeMutexLock clusterLock(pClient->mutex_);
    if(pClient->initState_ == REDIS_SVR_INVALID_STATE)//在这里由于内部资源已经处于释放状态，所以不需要继续往下走了
    {
        PERROR("asyFd %ld state %d", pClient->mgrFd_, pClient->initState_);
        return;
    }
    
    do{
        if(ret)
        {
            PERROR("asyFd %ld state %d ret %d", pClient->mgrFd_, pClient->initState_, ret);
            break;
        }
        REDIS_ASYNC_CLIENT::VectRedisClientPtr tmpAddRedisClis;
        int result = analysisClusterNodes(*resultMsg[0], *pClient, initIoIndex, tmpAddRedisClis, ioThreadNum_, keepSecond_, connOutSecond_);
        switch(result)
        {
            case 0:
                {
                    for(size_t i = 0; i < tmpAddRedisClis.size(); i++)
                    {
                        common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(tmpAddRedisClis[i]));
                        if(node == nullptr)
                        {
                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, EXCE_MALLOC_NULL, pClient->inSvrInfoStr_));
                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, pClient->mgrFd_));
                            return;
                        }
                        libeventIoPtrVect[tmpAddRedisClis[i]->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
                    }

                    std::vector<std::string> vLines;
                    split(*resultMsg[0], "\n", vLines);
                    for (size_t i = 0; i < vLines.size(); ++i)
                    {
                        std::string::size_type pos = vLines[i].find("master,fail");
                        if (pos != std::string::npos)
                        {
                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::clusterNodesUpdate, this, pClient->mgrFd_, true, false));
                            break;
                        }
                    }
                    return;
                }
            case ANALYSIS_STRING_FORMATE_ERROR:
                {
                    std::string errorMsg = pClient->inSvrInfoStr_ + "\n" + *resultMsg[0];
                    PERROR("asyFd %ld state %d result %d\n%s", pClient->mgrFd_, pClient->initState_, result, errorMsg.c_str());
                    callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, CLUSTER_INITSTR_FORMATE, errorMsg));
                    break;
                }
            case ANALYSIS_MALLOC_NULLPTR_ERROR:
                {
                    callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, EXCE_MALLOC_NULL, pClient->inSvrInfoStr_));
                    callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, pClient->mgrFd_));
                    return;
                }
            default:
                {
                    std::string errorMsg = pClient->inSvrInfoStr_ + "\n" + *resultMsg[0];
                    PERROR("asyFd %ld state %d result %d\n%s", pClient->mgrFd_, pClient->initState_, result, errorMsg.c_str());
                    callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, CLUSTER_INITSTR_UNKNOWN, errorMsg));
                    break;
                }
        }
    }while(0);
    callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::clusterNodesUpdate, this, pClient->mgrFd_, true, true));//延迟检查集群信息在这里做
}

void RedisAsync::infoReplicationCb(int ret, void* priv, const StdVectorStringPtr& resultMsg)
{
    PDEBUG("ret %d priv %p", ret , priv);
    for(StdVectorStringPtr::const_iterator iter = resultMsg.begin(); iter != resultMsg.end(); iter++)
    {
        PDEBUG("%s", (*iter)->c_str());
    }
    if(priv == nullptr)
    {
        return;
    }
    REDIS_ASYNC_CLIENT::RedisClientMgr *pClient = static_cast<REDIS_ASYNC_CLIENT::RedisClientMgr *>(priv);
    common::SafeMutexLock clusterLock(pClient->mutex_);
    if(pClient->initState_ == REDIS_SVR_INVALID_STATE)//在这里由于内部资源已经处于释放状态，所以不需要继续往下走了
    {
        PERROR("asyFd %ld state %d", pClient->mgrFd_, pClient->initState_);
        return;
    }

    REDIS_ASYNC_CLIENT::VectRedisClientPtr tmpAddRedisClis;
    int result = analysisSlaveNodes(*resultMsg[0], *pClient, initIoIndex, tmpAddRedisClis, ioThreadNum_, keepSecond_, connOutSecond_);
    if(result >= 0)
    {
        for(size_t i = 0; i < tmpAddRedisClis.size(); i++)
        {
            common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(tmpAddRedisClis[i]));
            if(node == nullptr)
            {
                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, EXCE_MALLOC_NULL, pClient->inSvrInfoStr_));
                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, pClient->mgrFd_));
                return;
            }
            libeventIoPtrVect[tmpAddRedisClis[i]->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
        }
        
        if((pClient->masterConn_ == 1) && (pClient->initState_ != REDIS_SVR_RUNING_STATE))
        {
            pClient->initState_ = REDIS_SVR_RUNING_STATE;
            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, pClient->mgrFd_, REDIS_SVR_RUNING_STATE, pClient->inSvrInfoStr_));
        }

        if((pClient->masterConn_ == 0) && (pClient->initState_ == REDIS_SVR_RUNING_STATE))
        {
            if(pClient->slaveConn_)
            {
                pClient->initState_ = REDIS_EXCEPTION_STATE;
                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, MASTER_SLAVE_NODES_CHANGE, pClient->inSvrInfoStr_));
                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, pClient->mgrFd_, REDIS_EXCEPTION_STATE, pClient->inSvrInfoStr_));
            }else{
                pClient->initState_ = REDIS_SVR_ERROR_STATE;
                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, MASTER_SLAVE_NODES_CHANGE, pClient->inSvrInfoStr_));
                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, pClient->mgrFd_, REDIS_SVR_ERROR_STATE, pClient->inSvrInfoStr_));
            }
        }
    }
    switch(result)
    {
        case 1:
            {
                break;
            }
        case 0:
            {
                return;
            }
        case ANALYSIS_STRING_FORMATE_ERROR:
            {
                break;
            }
        case ANALYSIS_MALLOC_NULLPTR_ERROR:
            {
                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, pClient->mgrFd_, EXCE_MALLOC_NULL, pClient->inSvrInfoStr_));
                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, pClient->mgrFd_));
                return;
            }
        default:
            {
                break;
            }
    }

    if((pClient->nodeCli_ == nullptr) || (pClient->nodeCli_->tcpCliState() == REDIS_CLIENT_STATE_INIT))
    {
        PDEBUG("pClient->nodeCli_");
        if(pClient->mSlaveCli_->master_ && (pClient->mSlaveCli_->master_->tcpCliState() == REDIS_CLIENT_STATE_CONN))
        {
            pClient->nodeCli_ = pClient->mSlaveCli_->master_;
        }

        for(size_t j = 0; j < pClient->mSlaveCli_->slave_.size(); j++)
        {
            if(pClient->mSlaveCli_->slave_[j] && (pClient->mSlaveCli_->slave_[j]->tcpCliState() == REDIS_CLIENT_STATE_CONN))
            {
                pClient->nodeCli_ = pClient->mSlaveCli_->slave_[j];
                break;
            }
        }
    }

    if(pClient->masterConn_)
    {
        PDEBUG("pClient->masterConn_ %ld", pClient->masterConn_);
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::infoReplicNodesUpdate, this, pClient->mgrFd_, false));
    }else{
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::infoReplicNodesUpdate, this, pClient->mgrFd_, true));
    }
    return;
}

int RedisAsync::addSigleRedisInfo(const std::string& ipPortInfo, const std::string& auth)
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
        PERROR("REDIS_ASYNC_CLIENT::RedisClientMgr nullptr");
        return INIT_SYSTEM_ERROR;
    }

    client->inSvrInfoStr_.assign(ipPortInfo);
    client->auth_.assign(auth);
    client->svrType_ = REDIS_SINGLE_SERVER;
    client->mgrFd_ = static_cast<int>(vectRedisCliMgr.size());
    asyFd = static_cast<int>(vectRedisCliMgr.size());
    vectRedisCliMgr.push_back(client);

    int ret = redisInitConnect();
    if(ret < 0)
    {
        PERROR("redisInitConnect %d", ret);
        return ret;
    }
    return asyFd;
}

int RedisAsync::addMasterSlaveInfo(const std::string& ipPortInfo, const std::string& auth)
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
        PERROR("REDIS_ASYNC_CLIENT::RedisClientMgr nullptr");
        return INIT_SYSTEM_ERROR;
    }

    client->inSvrInfoStr_.assign(ipPortInfo);
    client->auth_.assign(auth);
    client->svrType_ = REDIS_MASTER_SLAVE_SERVER;
    client->mgrFd_ = static_cast<int>(vectRedisCliMgr.size());
    asyFd = static_cast<int>(vectRedisCliMgr.size());
    vectRedisCliMgr.push_back(client);

    int ret = redisInitConnect();
    if(ret < 0)
    {
        PERROR("redisInitConnect %d", ret);
        return ret;
    }
    return asyFd;
}

int RedisAsync::addClusterInfo(const std::string& ipPortInfo, const std::string& auth)
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
        PERROR("REDIS_ASYNC_CLIENT::RedisClientMgr nullptr");
        return INIT_SYSTEM_ERROR;
    }

    client->inSvrInfoStr_.assign(ipPortInfo);
    client->auth_.assign(auth);
    client->svrType_ = REDIS_CLUSTER_SERVER;
    client->mgrFd_ = static_cast<int>(vectRedisCliMgr.size());
    asyFd = static_cast<int>(vectRedisCliMgr.size());
    vectRedisCliMgr.push_back(client);

    int ret = redisInitConnect();
    if(ret < 0)
    {
        PERROR("redisInitConnect %d", ret);
        return ret;
    }
    return asyFd;
}

void RedisAsync::updateNodesInfo(int asyFd)
{
    if(asyFd < 0)
    {
        return;
    }

    size_t mgrFd_ = static_cast<size_t>(asyFd);
    if(mgrFd_ >= vectRedisCliMgr.size())
    {
        return;
    }

    if(vectRedisCliMgr[asyFd]->initState_ == REDIS_SVR_INVALID_STATE)
    {
        return;
    }

    if(REDIS_MASTER_SLAVE_SERVER == vectRedisCliMgr[asyFd]->svrType_)
    {
        std::string infoReplic = "INFO replication";
        common::OrderNodePtr cmdnode(new common::OrderNode(infoReplic, std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::infoReplicationCb, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3)));
        if(cmdnode == nullptr)
        {
            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_MALLOC_NULL, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, asyFd));
            return;
        }
        cmdnode->outSecond_ = DEFAULT_CMD_OUTSECOND;
        cmdnode->cmdOutSecond_ = nowSecond_ + DEFAULT_CMD_OUTSECOND;
        common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(vectRedisCliMgr[asyFd]->nodeCli_, cmdnode, cmdnode->cmdOutSecond_));
        if(node == nullptr)
        {
            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_MALLOC_NULL, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, asyFd));
            return;
        }
        libeventIoPtrVect[vectRedisCliMgr[asyFd]->nodeCli_->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
    }else if(REDIS_CLUSTER_SERVER == vectRedisCliMgr[asyFd]->svrType_)
    {
    }

    return;
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
            PERROR("inSvrInfoStr %s", cli->inSvrInfoStr_.c_str());
            return INIT_PARAMETER_ERROR;//ip port格式不正确
        }

        len = pColon - pBegin;
        if(len > MAX_IP_ADDR_LEN)
        {
            PERROR("inSvrInfoStr %s", cli->inSvrInfoStr_.c_str());
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
            PERROR("inSvrInfoStr %s", cli->inSvrInfoStr_.c_str());
            return INIT_PARAMETER_ERROR;//ip port格式不正确
        }
        int port = atoi(pColon);

        REDIS_ASYNC_CLIENT::RedisSvrInfoPtr tmpSvrInfo(new REDIS_ASYNC_CLIENT::RedisSvrInfo(port, ipAddr));
        if(tmpSvrInfo == nullptr)
        {
            PERROR("REDIS_ASYNC_CLIENT::RedisSvrInfo nullptr");
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
        PERROR("REDIS_ASYNC_CLIENT::RedisClient nullptr");
        return INIT_SYSTEM_ERROR;
    }
    cli->nodeCli_ = tmpRedisCli;

    common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(tmpRedisCli));
    if(node == nullptr)
    {
        PERROR("common::RedisCliOrderNode nullptr");
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
void RedisAsync::redisSvrOnConnect(size_t asyFd, int state, const std::string& ipaddr, int port, const std::string errMsg)
{
    if((vectRedisCliMgr.size() <= asyFd) || (vectRedisCliMgr[asyFd] == nullptr))
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
                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, UNKOWN_REDIS_SVR_TYPE, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, asyFd));
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
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONNECT_RESET, exceMsgBuf));
                                        delayReconnect(asyFd, ipaddr, port);
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

                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONECT_DISCONNECT, exceMsgBuf));
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, asyFd, REDIS_SVR_ERROR_STATE, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
                                        reconnectAtOnece(asyFd, ipaddr, port);
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
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONNECT_RESET, exceMsgBuf));
                                        
                                        delayReconnect(asyFd, ipaddr, port);
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
                    case REDIS_SVR_INVALID_STATE:
                        {
                            PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
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
                common::SafeMutexLock clusterLock(vectRedisCliMgr[asyFd]->mutex_);
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
                                        if(vectRedisCliMgr[asyFd]->mSlaveCli_ == nullptr)
                                        {
                                            PDEBUG("INFO replication");
                                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::infoReplicNodesUpdate, this, asyFd, false));
                                        }else{
                                            int isNodes = infoReplicConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, true);
                                            if(isNodes == NO_IN_CLUSTER_ADDRINFO)
                                            {
                                                ;//TODO
                                            }

                                            if(vectRedisCliMgr[asyFd]->masterConn_)
                                            {
                                                vectRedisCliMgr[asyFd]->initState_ = REDIS_SVR_RUNING_STATE;
                                                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, asyFd, REDIS_SVR_RUNING_STATE, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
                                            }
                                        }
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONNECT_RESET, exceMsgBuf));
                                        
                                        delayReconnect(asyFd, ipaddr, port);
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONECT_DISCONNECT, exceMsgBuf));
                                        int isNodes = infoReplicConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, true);
                                        if(isNodes == NO_IN_CLUSTER_ADDRINFO)
                                        {
                                            ;//TODO
                                        }
                                        reconnectAtOnece(asyFd, ipaddr, port);
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
                                        int isNodes = infoReplicConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, true);
                                        if(isNodes == NO_IN_CLUSTER_ADDRINFO)
                                        {
                                            ;//TODO
                                        }
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONNECT_RESET, exceMsgBuf));
                                        
                                        delayReconnect(asyFd, ipaddr, port);
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONECT_DISCONNECT, exceMsgBuf));
                                        reconnectAtOnece(asyFd, ipaddr, port);
                                        
                                        int isNodes = infoReplicConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, true);
                                        if(isNodes == MASTER_CLUSTER_ADDRINFO)
                                        {
                                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::infoReplicNodesUpdate, this, asyFd, true));
                                        }
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
                    case REDIS_SVR_INVALID_STATE:
                        {
                            PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                            return;
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
                common::SafeMutexLock clusterLock(vectRedisCliMgr[asyFd]->mutex_);
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
                                        if((vectRedisCliMgr[asyFd]->cluterCli_ == nullptr) || (vectRedisCliMgr[asyFd]->cluterCli_->clusterVectInfo_.size() == 0))
                                        {
                                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::clusterNodesUpdate, this, asyFd, false, true));
                                        }else{
                                            int isClusterInfo = clusterRedisConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, true);
                                            if(isClusterInfo == NO_IN_CLUSTER_ADDRINFO)
                                            {
                                                ;//TODO
                                            }

                                            if(vectRedisCliMgr[asyFd]->cluterCli_->clusterVectInfo_.size() == vectRedisCliMgr[asyFd]->masterConn_)
                                            {
                                                vectRedisCliMgr[asyFd]->initState_ = REDIS_SVR_RUNING_STATE;
                                                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, asyFd, REDIS_SVR_RUNING_STATE, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
                                            }
                                        }
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONNECT_RESET, exceMsgBuf));

                                        int isClusterInfo = clusterRedisConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, false, false);
                                        if(isClusterInfo)
                                        {
                                            delayReconnect(asyFd, ipaddr, port);
                                            if(isClusterInfo == MASTER_CLUSTER_ADDRINFO)
                                            {
                                                vectRedisCliMgr[asyFd]->initState_ = REDIS_EXCEPTION_STATE;
                                                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, asyFd, REDIS_EXCEPTION_STATE, exceMsgBuf));
                                                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::clusterNodesUpdate, this, asyFd, true, false));
                                            }
                                        }
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONECT_DISCONNECT, exceMsgBuf));

                                        int isClusterInfo = clusterRedisConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, false);
                                        if(isClusterInfo)
                                        {
                                            reconnectAtOnece(asyFd, ipaddr, port);
                                            if(isClusterInfo == MASTER_CLUSTER_ADDRINFO)
                                            {
                                                vectRedisCliMgr[asyFd]->initState_ = REDIS_EXCEPTION_STATE;
                                                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, asyFd, REDIS_EXCEPTION_STATE, exceMsgBuf));
                                                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::clusterNodesUpdate, this, asyFd, true, false));
                                            }
                                        }
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
                                        int isClusterInfo = clusterRedisConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, true);
                                        if(isClusterInfo == MASTER_CLUSTER_ADDRINFO)
                                        {
                                            ;//TODO
                                        }
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONNECT_RESET, exceMsgBuf));

                                        delayReconnect(asyFd, ipaddr, port);
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONECT_DISCONNECT, exceMsgBuf));
                                        
                                        int isClusterInfo = clusterRedisConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, false);
                                        if(isClusterInfo)
                                        {
                                            reconnectAtOnece(asyFd, ipaddr, port);
                                            if(isClusterInfo == MASTER_CLUSTER_ADDRINFO)
                                            {
                                                vectRedisCliMgr[asyFd]->initState_ = REDIS_EXCEPTION_STATE;
                                                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, asyFd, REDIS_EXCEPTION_STATE, exceMsgBuf));
                                                callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::clusterNodesUpdate, this, asyFd, true, false));
                                            }
                                        }
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
                                        int isClusterInfo = clusterRedisConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, true);
                                        if(isClusterInfo == NO_IN_CLUSTER_ADDRINFO)
                                        {
                                            ;//TODO
                                        }
                                        
                                        if(vectRedisCliMgr[asyFd]->cluterCli_->clusterVectInfo_.size() == vectRedisCliMgr[asyFd]->masterConn_)
                                        {
                                            vectRedisCliMgr[asyFd]->initState_ = REDIS_SVR_RUNING_STATE;
                                            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, asyFd, REDIS_SVR_RUNING_STATE, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
                                        }
                                        
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONNECT_RESET, exceMsgBuf));

                                        delayReconnect(asyFd, ipaddr, port);
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONECT_DISCONNECT, exceMsgBuf));
                                        
                                        int isClusterInfo = clusterRedisConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, false);
                                        if(isClusterInfo)
                                        {
                                            reconnectAtOnece(asyFd, ipaddr, port);
                                        }
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
                                        int isClusterInfo = clusterRedisConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, true);
                                        if(isClusterInfo == NO_IN_CLUSTER_ADDRINFO)
                                        {
                                            ;//TODO
                                        }
                                        return;
                                    }
                                //function state 连接状态
                                case CONNECT_REDISVR_RESET:
                                    {
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONNECT_RESET, exceMsgBuf));

                                        delayReconnect(asyFd, ipaddr, port);
                                        return;
                                    }
                                //function state 连接状态
                                case REDISVR_CONNECT_DISCONN:
                                    {
                                        char exceMsgBuf[1024] = {0};
                                        sprintf(exceMsgBuf, "%s::%d %s", ipaddr.c_str(), port, errMsg.c_str());
                                        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, SVR_CONECT_DISCONNECT, exceMsgBuf));
                                        
                                        int isClusterInfo = clusterRedisConnsUpdate(vectRedisCliMgr[asyFd], ipaddr, port, false);
                                        if(isClusterInfo)
                                        {
                                            reconnectAtOnece(asyFd, ipaddr, port);
                                        }
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
                    case REDIS_SVR_INVALID_STATE:
                        {
                            PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
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
        default:
            {
                PERROR("asyFd %ld svrType_ %d inSvrInfoStr_ %s state %d svr info %s::%d", asyFd, vectRedisCliMgr[asyFd]->svrType_, vectRedisCliMgr[asyFd]->inSvrInfoStr_.c_str(), state, ipaddr.c_str(), port);
                return;
            }
    }
    return;
}

void RedisAsync::reconnectAtOnece(size_t asyFd, const std::string& ipaddr, int port)
{
    REDIS_ASYNC_CLIENT::RedisClientPtr client(nullptr);

    do{
        if(isSelfSvrInfo(vectRedisCliMgr[asyFd]->nodeCli_, ipaddr, port))
        {
            client = vectRedisCliMgr[asyFd]->nodeCli_;
            break;
        }

        client = getRedisClient(vectRedisCliMgr[asyFd]->mSlaveCli_, ipaddr, port);
        if(client)
        {
            break;
        }

        client = getRedisClient(vectRedisCliMgr[asyFd]->cluterCli_, ipaddr, port);
    }while(false);

    if(client == nullptr)
    {
        PERROR("asyFd %ld %s::%d", asyFd, ipaddr.c_str(), port);
        return;
    }
    common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(client));
    if(node == nullptr)
    {
        char exceMsgBuf[1024] = {0};
        sprintf(exceMsgBuf, "%s::%d", client->redisSvrIpaddr().c_str(), client->redisSvrPort());
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_MALLOC_NULL, exceMsgBuf));
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, asyFd));
        return;
    }
    common::TimerOrderDeque::instance().insertTimerOrder(node);
    return;
}

void RedisAsync::delayReconnect(size_t asyFd, const std::string& ipaddr, int port)
{
    REDIS_ASYNC_CLIENT::RedisClientPtr client(nullptr);

    do{
        if(isSelfSvrInfo(vectRedisCliMgr[asyFd]->nodeCli_, ipaddr, port))
        {
            client = vectRedisCliMgr[asyFd]->nodeCli_;
            break;
        }

        client = getRedisClient(vectRedisCliMgr[asyFd]->mSlaveCli_, ipaddr, port);
        if(client)
        {
            break;
        }

        client = getRedisClient(vectRedisCliMgr[asyFd]->cluterCli_, ipaddr, port);
    }while(false);

    if(client == nullptr)
    {
        PERROR("asyFd %ld %s::%d", asyFd, ipaddr.c_str(), port);
        return;
    }
    common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(client, (nowSecond_ + DEFAULT_DELAY_CMD_SECOND)));
    if(node == nullptr)
    {
        char exceMsgBuf[1024] = {0};
        sprintf(exceMsgBuf, "%s::%d", client->redisSvrIpaddr().c_str(), client->redisSvrPort());
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_MALLOC_NULL, exceMsgBuf));
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, asyFd));
        return;
    }
    common::TimerOrderDeque::instance().insertTimerOrder(node);
    return;
}

void RedisAsync::clusterNodesUpdate(size_t asyFd, bool isDelay, bool isInit)
{
    std::string clusterNodes = "CLUSTER NODES";
    common::OrderNodePtr cmdnode(nullptr);
    if(isInit)
    {
        cmdnode.reset(new common::OrderNode(clusterNodes, std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::clusterInitCallBack, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3), static_cast<void*>(vectRedisCliMgr[asyFd].get())));
    }else{
        cmdnode.reset(new common::OrderNode(clusterNodes, std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::clusterNodesCallBack, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3), static_cast<void*>(vectRedisCliMgr[asyFd].get())));
    }
    if(cmdnode == nullptr)
    {
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_MALLOC_NULL, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, asyFd));
        return;
    }
    if(isDelay)
    {
        common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(vectRedisCliMgr[asyFd]->nodeCli_, cmdnode, (nowSecond_ + DEFAULT_DELAY_CMD_SECOND)));
        if(node == nullptr)
        {
            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_MALLOC_NULL, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, asyFd));
            return;
        }
        common::TimerOrderDeque::instance().insertTimerOrder(node);
    }else{
        cmdnode->outSecond_ = DEFAULT_CMD_OUTSECOND;
        cmdnode->cmdOutSecond_ = nowSecond_ + DEFAULT_CMD_OUTSECOND;
        common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(vectRedisCliMgr[asyFd]->nodeCli_, cmdnode, cmdnode->cmdOutSecond_));
        if(node == nullptr)
        {
            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_MALLOC_NULL, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
            callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, asyFd));
            return;
        }
        libeventIoPtrVect[vectRedisCliMgr[asyFd]->nodeCli_->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
    }
    
}

void RedisAsync::infoReplicNodesUpdate(size_t asyFd, bool isDelay)
{
    std::string infoReplic = "INFO REPLICATION";
    common::OrderNodePtr cmdnode(new common::OrderNode(infoReplic, std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::infoReplicationCb, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3), static_cast<void*>(vectRedisCliMgr[asyFd].get())));
    if(cmdnode == nullptr)
    {
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_MALLOC_NULL, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, asyFd));
        return;
    }
    common::RedisCliOrderNodePtr node(nullptr);
    if(isDelay)
    {
        PDEBUG("isDelay true");
        node.reset(new common::RedisCliOrderNode(vectRedisCliMgr[asyFd]->nodeCli_, cmdnode, (nowSecond_ + DEFAULT_DELAY_CMD_SECOND)));
    }else{
        PDEBUG("isDelay false");
        node.reset(new common::RedisCliOrderNode(vectRedisCliMgr[asyFd]->nodeCli_, cmdnode));
    }
    if(node == nullptr)
    {
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncExceCallBack, this, asyFd, EXCE_MALLOC_NULL, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
        callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::redisSvrMgrInvalid, this, asyFd));
        return;
    }
    if(isDelay)
    {
        common::TimerOrderDeque::instance().insertTimerOrder(node);
    }else{
        libeventIoPtrVect[vectRedisCliMgr[asyFd]->nodeCli_->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
    }
}

void RedisAsync::redisSvrMgrInvalid(size_t asyFd)
{
    common::SafeMutexLock clusterLock(vectRedisCliMgr[asyFd]->mutex_);

    if(vectRedisCliMgr[asyFd]->nodeCli_)
    {
        vectRedisCliMgr[asyFd]->nodeCli_->setReleaseState();
    }

    if(vectRedisCliMgr[asyFd]->mSlaveCli_)
    {
        if(vectRedisCliMgr[asyFd]->mSlaveCli_->master_)
        {
            vectRedisCliMgr[asyFd]->mSlaveCli_->master_->setReleaseState();
        }

        for(size_t i = 0; i < vectRedisCliMgr[asyFd]->mSlaveCli_->slave_.size(); i++)
        {
            if(vectRedisCliMgr[asyFd]->mSlaveCli_->slave_[i])
            {
                vectRedisCliMgr[asyFd]->mSlaveCli_->slave_[i]->setReleaseState();
            }
        }

        vectRedisCliMgr[asyFd]->mSlaveCli_.reset();
    }
    
    if(vectRedisCliMgr[asyFd]->cluterCli_)
    {
        for(size_t i = 0; i < vectRedisCliMgr[asyFd]->cluterCli_->clusterVectInfo_.size(); i++)
        {
            if(vectRedisCliMgr[asyFd]->cluterCli_->clusterVectInfo_[i] && vectRedisCliMgr[asyFd]->cluterCli_->clusterVectInfo_[i]->clusterNode_)
            {
                if(vectRedisCliMgr[asyFd]->cluterCli_->clusterVectInfo_[i]->clusterNode_->master_)
                {
                    vectRedisCliMgr[asyFd]->cluterCli_->clusterVectInfo_[i]->clusterNode_->master_->setReleaseState();
                }

                for(size_t j = 0; j < vectRedisCliMgr[asyFd]->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_.size(); j++)
                {
                    if(vectRedisCliMgr[asyFd]->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_[j])
                    {
                        vectRedisCliMgr[asyFd]->cluterCli_->clusterVectInfo_[i]->clusterNode_->slave_[j]->setReleaseState();
                    }
                }
            }
        }

        vectRedisCliMgr[asyFd]->cluterCli_.reset();
    }
    
    vectRedisCliMgr[asyFd]->initState_ = REDIS_SVR_INVALID_STATE;
    callBackPoolPtr->run(std::bind(&CLUSTER_REDIS_ASYNC::RedisAsync::asyncStateCallBack, this, asyFd, REDIS_SVR_INVALID_STATE, vectRedisCliMgr[asyFd]->inSvrInfoStr_));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int RedisAsync::redisAsyncCommand(int asyFd, const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const std::string& cmdStr)
{
    REDIS_ASYNC_CLIENT::RedisClientPtr redisClient(nullptr);
    if(asyFd < 0 || key.empty() || cmdStr.empty() || (outSecond == 0))
    {
        return CMD_PARAMETER_ERROR_CODE;
    }

    size_t mgrFd_ = static_cast<size_t>(asyFd);
    if(mgrFd_ >= vectRedisCliMgr.size())
    {
        return CMD_PARAMETER_ERROR_CODE;
    }

    if(vectRedisCliMgr[mgrFd_] == nullptr)
    {
        return CMD_SVR_INVALID_CODE;
    }

    if((vectRedisCliMgr[mgrFd_]->initState_ == REDIS_SVR_INIT_STATE) || (vectRedisCliMgr[mgrFd_]->initState_ == REDIS_SVR_ERROR_STATE)|| (vectRedisCliMgr[mgrFd_]->initState_ == REDIS_SVR_INVALID_STATE))
    {
        return CMD_SVR_STATE_ERROR_CODE;
    }

    switch(vectRedisCliMgr[mgrFd_]->svrType_)
    {
        case REDIS_UNKNOWN_SERVER:
            {
                return CMD_SVR_INVALID_CODE;
            }
        case REDIS_SINGLE_SERVER:
            {
                redisClient = vectRedisCliMgr[mgrFd_]->nodeCli_;
                break;
            }
        case REDIS_MASTER_SLAVE_SERVER:
            {
                redisClient = vectRedisCliMgr[mgrFd_]->nodeCli_;
                break;
            }
        case REDIS_CLUSTER_SERVER:
            {
                uint16_t slot = getKeySlotIndex(key);
                if(slot > REDIS_CLUSTER_MAX_SLOT)
                {
                    return CMD_SLOTS_CALCUL_ERROR_CODE;
                }
                common::SafeMutexLock clusterLock(vectRedisCliMgr[asyFd]->mutex_);
                if(vectRedisCliMgr[mgrFd_]->cluterCli_)
                {
                    REDIS_ASYNC_CLIENT::SlotCliInfoMap::iterator iter = vectRedisCliMgr[mgrFd_]->cluterCli_->clusterSlotMap_.find(slot);
                    if(iter == vectRedisCliMgr[mgrFd_]->cluterCli_->clusterSlotMap_.end())
                    {
                        return CMD_SVR_NODES_DOWN;
                    }
                    redisClient = iter->second;
                    if((redisClient == nullptr) || (redisClient->tcpCliState() == REDIS_CLIENT_STATE_INIT))
                    {
                        redisClient = getRedisClient(vectRedisCliMgr[mgrFd_], slot);
                    }
                }else{
                    return CMD_SVR_INVALID_CODE;
                }
                break;
            }
        default:
            {
                return CMD_SVR_INVALID_CODE;
            }
    }

    if((redisClient == nullptr) || (redisClient->tcpCliState() == REDIS_CLIENT_STATE_INIT))
    {
        return CMD_SVR_NODES_DOWN;
    }

    common::OrderNodePtr cmdOrder(new common::OrderNode(cmdStr, cb, priv));
    if(cmdOrder == nullptr)
    {
        return CMD_SYSTEM_MALLOC_CODE;
    }
    cmdOrder->outSecond_ = outSecond;
    cmdOrder->cmdOutSecond_ = nowSecond_ + outSecond;
    common::RedisCliOrderNodePtr node(new common::RedisCliOrderNode(redisClient, cmdOrder, cmdOrder->cmdOutSecond_));
    if(node == nullptr)
    {
        return CMD_SYSTEM_MALLOC_CODE;
    }
    libeventIoPtrVect[redisClient->redisCliIoIndex()]->libeventIoOrder(node, nowSecond_);
    return 0;
}

}

