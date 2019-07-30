
#include <unistd.h>
#include <malloc.h>
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
//#pragma GCC diagnostic ignored "-fpermissive"

#include <adapters/libevent.h>

#include "Incommon.h"
#include "RedisClient.h"

namespace REDIS_ASYNC_CLIENT
{

static void connectTimeout(evutil_socket_t fd, short event, void *arg)
{
    PTRACE("arg %p", arg);
    RedisClient* pClient = static_cast<RedisClient*>(arg);
    if(pClient->tcpCliState() == REDIS_CLIENT_STATE_INIT)
    {
        pClient->disConnect(true);
        std::string ipaddr = pClient->redisSvrIpaddr();
        int port = pClient->redisSvrPort();
        CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisSvrOnConnect(pClient->redisMgrfd(), CONNECT_REDISVR_RESET, ipaddr, port);
    }
    return;
}

static void connectCallback(const redisAsyncContext *c, int status)
{
    PTRACE("redisAsyncContext %p status %d", c, status);
    RedisClient* pClient = static_cast<RedisClient*>(c->data);
    std::string ipaddr = pClient->redisSvrIpaddr();
    int port = pClient->redisSvrPort();
    if (status != REDIS_OK)
    {
        PERROR("Error %s", c->errstr);
        if(pClient->tcpCliState() == REDIS_CLIENT_STATE_INIT)
        {
            pClient->disConnect();
            CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisSvrOnConnect(pClient->redisMgrfd(), CONNECT_REDISVR_RESET, ipaddr, port, c->errstr);
        }else{
            pClient->disConnect();
            CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisSvrOnConnect(pClient->redisMgrfd(), REDISVR_CONNECT_DISCONN, ipaddr, port, c->errstr);
        }
        return;
    }
    pClient->setStateConnected();
    CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisSvrOnConnect(pClient->redisMgrfd(), CONNECT_REDISVR_SUCCESS, ipaddr, port);
    PDEBUG("%s::%d Connected....", pClient->redisSvrIpaddr().c_str(), pClient->redisSvrPort());
    return;
}

static void disconnectCallback(const redisAsyncContext *c, int status)
{
    PTRACE("redisAsyncContext %p status %d", c, status);
    if (status != REDIS_OK)
    {
        PERROR("Error %s", c->errstr);
    }
    RedisClient* pClient = static_cast<RedisClient*>(c->data);
    int state = pClient->tcpCliState();
    pClient->disConnect();
    std::string ipaddr = pClient->redisSvrIpaddr();
    int port = pClient->redisSvrPort();
    if(state == REDIS_CLIENT_STATE_INIT)
    {
        CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisSvrOnConnect(pClient->redisMgrfd(), CONNECT_REDISVR_RESET, ipaddr, port, c->errstr);
    }else{
        CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisSvrOnConnect(pClient->redisMgrfd(), REDISVR_CONNECT_DISCONN, ipaddr, port, c->errstr);
    }
    PDEBUG("%s::%d Disconnected....", ipaddr.c_str(), port);
    return;
}

static void cmdCallback(redisAsyncContext *c, void *r, void *privdata)
{
    RedisClient* pClient = static_cast<RedisClient*>(c->data);
    redisReply* reply = static_cast<redisReply*>(r);
    if (reply == NULL)
    {
        PERROR("cmdCallback reply nullptr");
        return;
    }
    pClient->requestCallBack(privdata, reply);
    return;
}

RedisClient::RedisClient(size_t ioIndex, size_t fd, const RedisSvrInfoPtr& svrInfo, int keepAliveSecond, int connOutSecond)
    :state_(REDIS_CLIENT_STATE_INIT)
    ,connOutSecond_(connOutSecond)
    ,keepAliveSecond_(keepAliveSecond)
    ,releaseState_(0)
    ,freeState_(RESOURCES_FREE_ALREADY)
    ,ioIndex_(ioIndex)
    ,mgrFd_(fd)
    ,lastSecond_(0)
    ,timev_(nullptr)
    ,base_(nullptr)
    ,client_(nullptr)
    ,svrInfo_(svrInfo)
    ,cmdSeq_()
    ,cmdSeqOrderMap_()
{
    PDEBUG("RedisClient init");
}

//这个类除了new之外所有的操作都应该在一个线程中完成，千万不要跨线程
RedisClient::~RedisClient()
{
    PERROR("~RedisClient exit");
    if(freeState_ == RESOURCES_NEED_FREE)
    {
        clearCmdReqBuf();
    }
    freeState_ = RESOURCES_FREE_ALREADY;
    freeTimeEvent();
    if(client_)
    {
        redisAsyncDisconnect(client_);//这里会触发断连回调
        client_ = NULL;
    }
    if(svrInfo_)
    {
        svrInfo_.reset();
    }

    common::SeqOrderNodeMap ().swap(cmdSeqOrderMap_);
    base_ = NULL;
    state_ = REDIS_CLIENT_STATE_INIT;
}

int RedisClient::connectServer(struct event_base* eBase)
{
    if((eBase == nullptr) || (svrInfo_ == nullptr) || (svrInfo_->ipAddr_.empty()) || (svrInfo_->port_ == 0))
    {
        PERROR("eBase nullptr or svrInfo_ nullptr or ipaddr port empty");
         return -1;
    }
    //disConnect(true);
    base_ = eBase;
    client_ = redisAsyncConnect(svrInfo_->ipAddr_.c_str(), svrInfo_->port_);
    if(client_ == nullptr)
    {
        PERROR("redisAsyncConnect nullptr");
        return -1;
    }
    if (client_->err)
    {
         PERROR("redisAsyncConnect client_->err %d", client_->err);
         return -1;
    }
    
    client_->data = static_cast<void*>(this);
    redisAsyncSetConnectCallback(client_, connectCallback);
    redisAsyncSetDisconnectCallback(client_, disconnectCallback);
    redisLibeventAttach(client_, base_);

    timev_ = evtimer_new(base_, connectTimeout, static_cast<void*>(this));
    if(timev_ == nullptr)
    {
         PERROR("evtimer_new malloc null");
         return -1;
    }
    freeState_ = RESOURCES_NEED_FREE;
    struct timeval connSecondOut = {connOutSecond_, 0};
    evtimer_add(timev_, &connSecondOut);// call back outtime check
    PTRACE("%s::%d eBase %p ", svrInfo_->ipAddr_.c_str(), svrInfo_->port_, base_);
    return 0;
}

void RedisClient::disConnect(bool isFree)
{
    PTRACE("disConnect entry");
    if(freeState_ == RESOURCES_FREE_ALREADY)
    {
        return;
    }
    freeState_ = RESOURCES_FREE_ALREADY;
    PTRACE("timer event free");
    freeTimeEvent();
    PTRACE("redis client free");
    if(client_ && isFree)
    {
        redisAsyncDisconnect(client_);//这里会触发断连回调
        client_ = NULL;
    }
    PTRACE("disConnect free");
    clearCmdReqBuf();
    state_ = REDIS_CLIENT_STATE_INIT;
}

void RedisClient::requestCmd(const common::OrderNodePtr& order, uint64_t nowSecond)
{
    uint32_t seq = cmdSeq_.getAndIncrement();
    void *priv = (void *)(seq);
    PDEBUG("requestCmd %u::%p", seq, priv);
    int ret = redisAsyncCommand(client_, cmdCallback, priv, order->cmdMsg_.c_str());
    if(ret)
    {
        char errorMsg[1024] = {0};
        sprintf(errorMsg, "redisAsyncCommand ret %d", ret);
        CLUSTER_REDIS_ASYNC::StdStringSharedPtr errorStr(new std::string(errorMsg));
        if(errorStr)
        {
            order->cmdResult_.push_back(errorStr);
        }
        order->cmdRet_ = ret;
        common::CmdResultQueue::instance().insertCmdResult(order);
        return;
    }
    lastSecond_ = nowSecond;
    order->cmdQuerySecond_ = nowSecond;
    cmdSeqOrderMap_.insert(common::SeqOrderNodeMap::value_type(seq, order));
}

void RedisClient::clearCmdReqBuf()
{
    for(common::SeqOrderNodeMap::iterator iter = cmdSeqOrderMap_.begin(); iter != cmdSeqOrderMap_.end();iter++)
    {
        if(iter->second && iter->second->resultCb_)
        {
            iter->second->cmdRet_ = CMD_SVR_CLOSER_CODE;
            common::CmdResultQueue::instance().insertCmdResult(iter->second);
        }else{
            PERROR("no callback %u", iter->first);
        }
    }
}

void RedisClient::checkOutSecondCmd(uint64_t nowSecond)
{
    if(nowSecond == 0)
    {
        nowSecond = secondSinceEpoch();
    }
    
    if((state_ == REDIS_CLIENT_STATE_CONN) && (static_cast<int>(nowSecond - lastSecond_) > keepAliveSecond_))
    {
        redisAsyncCommand(client_, nullptr, nullptr, "PING");//不需要回调，靠回调来检查连接正常与否也是很难界定的
        lastSecond_ = nowSecond;
    }
    
    if(cmdSeqOrderMap_.empty())
    {
        return;
    }

    for(common::SeqOrderNodeMap::iterator iter = cmdSeqOrderMap_.begin(); iter != cmdSeqOrderMap_.end();)
    {
        if(iter->second->outSecond_ && (nowSecond > iter->second->cmdOutSecond_))
        {
            if(iter->second && iter->second->resultCb_)
            {
                iter->second->cmdRet_ = CMD_OUTTIME_CODE;
                common::CmdResultQueue::instance().insertCmdResult(iter->second);
            }else{
                PERROR("no callback %u", iter->first);
            }
            cmdSeqOrderMap_.erase(iter++);
        }else{
            iter++;
        }
    }
}

void RedisClient::requestCallBack(void* priv, redisReply* reply)
{
    uint64_t tmpSeq = (uint64_t)priv;
    uint32_t seq = (uint32_t)tmpSeq;
    PDEBUG("requestCallBack %u::%p", seq, priv);
    common::SeqOrderNodeMap::iterator iter = cmdSeqOrderMap_.find(seq);
    if(iter == cmdSeqOrderMap_.end())
    {
        PERROR("callback been outtime or no callback %u", seq);
    }else{
        if(iter->second)
        {
            if(reply == NULL)
            {
                if(iter->second->resultCb_)
                {
                    iter->second->cmdRet_ = CMD_REPLY_EMPTY_CODE;
                    common::CmdResultQueue::instance().insertCmdResult(iter->second);
                }else{
                    PERROR("no callback %u", seq);
                }
            }else{
                switch(reply->type)
                {
                    case REDIS_REPLY_STRING:
                        {
                            PDEBUG("REDIS_REPLY_STRING %d", REDIS_REPLY_STRING);
                            if(iter->second->resultCb_)
                            {
                                CLUSTER_REDIS_ASYNC::StdStringSharedPtr elementStr(new std::string(reply->str, reply->len));
                                if(elementStr)
                                {
                                    iter->second->cmdResult_.push_back(elementStr);
                                    iter->second->cmdRet_ = CMD_SUCCESS_CODE;
                                }else{
                                    iter->second->cmdRet_ = CMD_SYSTEM_MALLOC_CODE;
                                }
                                common::CmdResultQueue::instance().insertCmdResult(iter->second);
                            }else{
                                PERROR("no callback %u", seq);
                            }
                            break;
                        }
                    case REDIS_REPLY_ARRAY:
                        {
                            PDEBUG("REDIS_REPLY_ARRAY %d", REDIS_REPLY_ARRAY);
                            if(iter->second->resultCb_)
                            {
                                iter->second->cmdRet_ = CMD_SUCCESS_CODE;
                                for (size_t j = 0; j < reply->elements; j++)
                                {
                                    CLUSTER_REDIS_ASYNC::StdStringSharedPtr elementStr(new std::string(reply->element[j]->str, reply->element[j]->len));
                                    if(elementStr == nullptr)
                                    {
                                        iter->second->cmdRet_ = CMD_SYSTEM_MALLOC_CODE;
                                        iter->second->cmdResult_.push_back(CLUSTER_REDIS_ASYNC::StdStringSharedPtr());
                                    }else{
                                        iter->second->cmdResult_.push_back(elementStr);
                                    }
                                }
                                common::CmdResultQueue::instance().insertCmdResult(iter->second);
                            }else{
                                PERROR("no callback %u", seq);
                            }
                            break;
                        }
                    case REDIS_REPLY_INTEGER:
                        {
                            PDEBUG("REDIS_REPLY_INTEGER %d", REDIS_REPLY_INTEGER);
                            if(iter->second->resultCb_)
                            {
                                iter->second->cmdRet_ = CMD_SUCCESS_CODE;
                                std::string tmpString = std::to_string(reply->integer);
                                CLUSTER_REDIS_ASYNC::StdStringSharedPtr elementStr(new std::string(tmpString));
                                if(elementStr)
                                {
                                    iter->second->cmdResult_.push_back(elementStr);
                                }else{
                                    iter->second->cmdRet_ = CMD_SYSTEM_MALLOC_CODE;
                                }
                                common::CmdResultQueue::instance().insertCmdResult(iter->second);
                            }else{
                                PERROR("no callback %u", seq);
                            }
                            break;
                        }
                    case REDIS_REPLY_NIL:
                        {
                            PDEBUG("REDIS_REPLY_NIL %d", REDIS_REPLY_NIL);
                            if(iter->second->resultCb_)
                            {
                                iter->second->cmdRet_ = CMD_EMPTY_RESULT_CODE;
                                common::CmdResultQueue::instance().insertCmdResult(iter->second);
                            }else{
                                PERROR("no callback %u", seq);
                            }
                            break;
                        }
                    case REDIS_REPLY_STATUS:
                        {
                            PDEBUG("REDIS_REPLY_STATUS %d", REDIS_REPLY_STATUS);
                            if(iter->second->resultCb_)
                            {
                                iter->second->cmdRet_ = CMD_SUCCESS_CODE;
                                std::string tmpString = std::to_string(reply->integer);
                                CLUSTER_REDIS_ASYNC::StdStringSharedPtr elementStr(new std::string(tmpString));
                                if(elementStr)
                                {
                                    iter->second->cmdResult_.push_back(elementStr);
                                }else{
                                    iter->second->cmdRet_ = CMD_SYSTEM_MALLOC_CODE;
                                }
                                common::CmdResultQueue::instance().insertCmdResult(iter->second);
                            }else{
                                PERROR("no callback %u", seq);
                            }
                            break;
                        }
                    case REDIS_REPLY_ERROR:
                        {
                            PDEBUG("REDIS_REPLY_ERROR %d", REDIS_REPLY_ERROR);
                            if(iter->second->resultCb_)
                            {
                                iter->second->cmdRet_ = CMD_REDIS_ERROR_CODE;
                                CLUSTER_REDIS_ASYNC::StdStringSharedPtr elementStr(new std::string(reply->str, reply->len));
                                if(elementStr)
                                {
                                    iter->second->cmdResult_.push_back(elementStr);
                                }else{
                                    iter->second->cmdRet_ = CMD_SYSTEM_MALLOC_CODE;
                                }
                                common::CmdResultQueue::instance().insertCmdResult(iter->second);
                            }else{
                                PERROR("no callback %u", seq);
                            }
                            break;
                        }
                    default:
                        {
                            PDEBUG("default %d", reply->type);
                            if(iter->second->resultCb_)
                            {
                                iter->second->cmdRet_ = CMD_REDIS_UNKNOWN_CODE;
                                for (size_t j = 0; j < reply->elements; j++)
                                {
                                    CLUSTER_REDIS_ASYNC::StdStringSharedPtr elementStr(new std::string(reply->element[j]->str, reply->element[j]->len));
                                    if(elementStr == nullptr)
                                    {
                                        iter->second->cmdResult_.push_back(CLUSTER_REDIS_ASYNC::StdStringSharedPtr());
                                    }else{
                                        iter->second->cmdResult_.push_back(elementStr);
                                    }
                                }
                                common::CmdResultQueue::instance().insertCmdResult(iter->second);
                            }else{
                                PERROR("no callback %u", seq);
                            }
                            break;
                        }
                }
            }
        }else{
            PERROR("no callback %u", seq);
        }
        cmdSeqOrderMap_.erase(iter);
    }
}

}

