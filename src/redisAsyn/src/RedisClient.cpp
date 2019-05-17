
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
    RedisClient* pClient = static_cast<RedisClient*>(arg);
    if(pClient->tcpCliState() == REDIS_CLIENT_STATE_INIT)
    {
        pClient->disConnect();
        std::string ipaddr = pClient->redisSvrIpaddr();
        int port = pClient->redisSvrPort();
        CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisSvrOnConnect(pClient->redisMgrfd(), CONNECT_REDISVR_RESET, ipaddr, port);
    }
    return;
}

static void connectCallback(const redisAsyncContext *c, int status)
{
    RedisClient* pClient = static_cast<RedisClient*>(c->data);
    std::string ipaddr = pClient->redisSvrIpaddr();
    int port = pClient->redisSvrPort();
    if (status != REDIS_OK)
    {
        PERROR("Error %s", c->errstr);
        if(pClient->tcpCliState() == REDIS_CLIENT_STATE_INIT)
        {
            pClient->disConnect();
            CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisSvrOnConnect(pClient->redisMgrfd(), CONNECT_REDISVR_RESET, ipaddr, port);
        }else{
            pClient->disConnect();
            CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisSvrOnConnect(pClient->redisMgrfd(), REDISVR_CONNECT_DISCONN, ipaddr, port);
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
    if (status != REDIS_OK)
    {
        PERROR("Error %s", c->errstr);
        //return;//TODO
    }
    RedisClient* pClient = static_cast<RedisClient*>(c->data);
    pClient->disConnect();
    std::string ipaddr = pClient->redisSvrIpaddr();
    int port = pClient->redisSvrPort();
    if(pClient->tcpCliState() == REDIS_CLIENT_STATE_INIT)
    {
        CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisSvrOnConnect(pClient->redisMgrfd(), CONNECT_REDISVR_RESET, ipaddr, port);
    }else{
        CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisSvrOnConnect(pClient->redisMgrfd(), REDISVR_CONNECT_DISCONN, ipaddr, port);
    }
    PDEBUG("Disconnected....");
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
}

RedisClient::~RedisClient()
{
    if(timev_)
    {
        event_free(timev_);
        timev_ = nullptr;
    }
    if(client_)
    {
        redisAsyncDisconnect(client_);
        client_ = NULL;
    }
    base_ = NULL;
}

int RedisClient::connectServer(struct event_base* eBase)
{
    if((eBase == nullptr) || (svrInfo_ == nullptr) || (svrInfo_->ipAddr_.empty()) || (svrInfo_->port_ == 0))
    {
        PERROR("eBase nullptr or svrInfo_ nullptr or ipaddr port empty");
         return -1;
    }
    disConnect();
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

    struct timeval connSecondOut = {connOutSecond_, 0};
    evtimer_add(timev_, &connSecondOut);// call back outtime check
    return 0;
}

void RedisClient::disConnect()
{
    if(timev_)
    {
        event_free(timev_);
        timev_ = nullptr;
    }
    if(client_)
    {
        redisAsyncDisconnect(client_);
        client_ = NULL;
    }

    state_ = REDIS_CLIENT_STATE_INIT;
}

void RedisClient::requestCmd(const common::OrderNodePtr& order)
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

    order->cmdQuerySecond_ = secondSinceEpoch();
    cmdSeqOrderMap_.insert(common::SeqOrderNodeMap::value_type(seq, order));
}

void RedisClient::checkOutSecondCmd(uint64_t nowSecond)
{
    if(cmdSeqOrderMap_.empty())
    {
        return;
    }

    if(nowSecond == 0)
    {
        nowSecond = secondSinceEpoch();
    }

    for(common::SeqOrderNodeMap::iterator iter = cmdSeqOrderMap_.begin(); iter != cmdSeqOrderMap_.end(); iter++)
    {
        if(iter->second->outSecond_ && (nowSecond > iter->second->cmdOutSecond_))
        {
            if(iter->second->resultCb_)
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

