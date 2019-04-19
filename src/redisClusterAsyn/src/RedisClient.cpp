
#include <unistd.h>
#include <malloc.h>
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
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
        ;
    }
    return;
}

static void connectCallback(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK)
    {
        PERROR("Error %s", c->errstr);
        return;
    }
    RedisClient* pClient = static_cast<RedisClient*>(c->data);
    pClient->setStateConnected();
    PDEBUG("Connected....");
    return;
}

static void disconnectCallback(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK)
    {
        PERROR("Error %s", c->errstr);
        return;
    }
    RedisClient* pClient = static_cast<RedisClient*>(c->data);
    if(pClient->tcpCliState() == REDIS_CLIENT_STATE_INIT)
    {
        ;
    }else{
        ;
    }
    PDEBUG("Disconnected....");
    return;
}

static void cmdCallback(redisAsyncContext *c, void *r, void *privdata)
{
    redisReply* reply = static_cast<redisReply*>(r);
    if (reply == NULL)
    {
        return;
    }
    return;
}

RedisClient::RedisClient(size_t ioIndex, const std::string& ipaddr, int port, int cmdOutSecond, int connOutSecond)
    :state_(0)
    ,port_(port)
    ,connOutSecond_(connOutSecond)
    ,dataOutSecond_(cmdOutSecond)
    ,ioIndex_(ioIndex)
    ,timev_(nullptr)
    ,base_(nullptr)
    ,client_(nullptr)
    ,ipaddr_(ipaddr)
{
}

RedisClient::~RedisClient()
{
}

int RedisClient::connectServer(struct event_base* eBase)
{
    if(eBase == nullptr)
    {
        return -101;
    }
    base_ = eBase;
    client_ = redisAsyncConnect(ipaddr_.c_str(), port_);
    if(client_ == nullptr)
    {
        PERROR("TODO");
        return -102;
    }
    if (client_->err)
    {
        PERROR("TODO");
        return -103;
    }
    
    client_->data = static_cast<void*>(this);
    redisAsyncSetConnectCallback(client_, connectCallback);
    redisAsyncSetDisconnectCallback(client_, disconnectCallback);
    redisLibeventAttach(client_, base_);

    timev_ = evtimer_new(base_, connectTimeout, static_cast<void*>(this));
    if(timev_ == nullptr)
    {
        PERROR("evtimer_new malloc null");
        return -104;
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
}

int RedisClient::requestCmd(const std::string& cmd, void* priv)
{
    if(REDIS_CLIENT_STATE_CONN == state_)
    {
        return redisAsyncCommand(client_, cmdCallback, priv, cmd.c_str());
    }
    return -105;
}


}

