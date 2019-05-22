
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/cdefs.h>

#include <unistd.h>

#include "Incommon.h"
#include "LibeventIo.h"

#pragma GCC diagnostic ignored "-Wold-style-cast"
namespace common
{
static int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        PERROR("Failed in eventfd");
    }
    return evtfd;
}

static void wakeUpFdcb(int sockFd, short eventType, void *arg)
{
    LibeventIo *p = (LibeventIo *)arg;
    p->handleRead();
}

LibeventIo::LibeventIo()
    :wakeupFd(createEventfd())
    ,evbase(nullptr)
    ,nowSecond_(0)
    ,lastSecond_(0)
    ,wake_event()
    ,orderDeque_()
    ,ioRedisClients_()
{
    PDEBUG("LibeventIo init");
}

LibeventIo::~LibeventIo()
{
    PERROR("~LibeventIo exit");
}

int LibeventIo::libeventIoReady()
{
    PDEBUG("libeventIoReady in");
    if(wakeupFd < 0)
    {
        PERROR("libeventIoReady wakeupFd_ error %d", wakeupFd);
        return -1;
    }

    evbase = event_base_new();
    if(evbase == nullptr)
    {
        PERROR("libeventIoReady event_base_new new error");
        return -1;
    }

    event_assign(&wake_event, evbase, wakeupFd, EV_READ|EV_PERSIST, wakeUpFdcb, this);
    event_add(&wake_event, NULL);

    event_base_dispatch(evbase);
    PERROR("event_base_dispatch return %p", evbase);
    //正常情况下这句话根本就不会返回。如果返回了，其实我也不知道为什么

    event_del(&wake_event);
    close(wakeupFd);
    event_base_free(evbase);
    evbase = nullptr;
    return 0;
}

int LibeventIo::libeventIoOrder(const RedisCliOrderNodePtr& node, uint64_t nowsecond)
{
    PDEBUG("nowsecond %ld", nowsecond);
    if(node == nullptr)
    {
        return 0;
    }
    if(node->cmdOrd_ && node->cmdOrd_->resultCb_)
    {
        CLUSTER_REDIS_ASYNC::RedisAsync::instance().asyncRequestCmdAdd();
    }
    orderDeque_.orderNodeInsert(node);
    libeventIoWakeup(nowsecond);
    return 0;
}

int LibeventIo::libeventIoWakeup(uint64_t nowsecond)
{
    uint64_t one = 2;
    nowSecond_ = nowsecond;
    ssize_t n = write(wakeupFd, &one, sizeof one);
    //PDEBUG("wakeup n one %ld %ld %p", n, one, evbase);
    if (n != sizeof one)
    {
        PERROR("EventLoop::wakeup() writes %ld bytes instead of 8", n);
    }
    return 0;
}

int LibeventIo::libeventIoExit()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd, &one, sizeof one);
    PDEBUG("LibeventIo n one %ld %ld %p", n, one, evbase);
    if (n != sizeof one)
    {
        PERROR("EventLoop::wakeup() writes %ld bytes instead of 8", n);
    }
    return 0;
}

//不需要删除，因为一个redis客户端绑定的io之后的重连无论任何操作绑定的io线程不应该改变
//当redis管理出现不可修复错误时，需要整个析构的时候，需要掉用这个，析构内部所有的redis client对象
void LibeventIo::ioDisRedisClient(const REDIS_ASYNC_CLIENT::RedisClientPtr& cli)
{
    for(size_t i = 0; i < ioRedisClients_.size(); i++)
    {
        if(ioRedisClients_[i].get() == cli.get())
        {
            ioRedisClients_[i].reset();
        }
    }
}

void LibeventIo::ioAddRedisClient(const REDIS_ASYNC_CLIENT::RedisClientPtr& cli)
{
    for(size_t i = 0; i < ioRedisClients_.size(); i++)
    {
        if(ioRedisClients_[i].get() == cli.get())
        {
            return;
        }
    }

    for(size_t i = 0; i < ioRedisClients_.size(); i++)
    {
        if(ioRedisClients_[i] == nullptr)
        {
            ioRedisClients_[i] = cli;
        }
    }

    ioRedisClients_.push_back(cli);
}

void LibeventIo::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd, &one, sizeof one);
    if (n != sizeof one)
    {
        PERROR("LibeventIo::handleRead() reads %ld bytes instead of 8", n);
        return;
    }
    //PDEBUG("handleRead n one %ld %ld :: %p", n, one, evbase);

    while(1)
    {
        RedisCliOrderNodePtr node = orderDeque_.dealOrderNode();
        if(node)
        {
            if((node->outSecond_) && (nowSecond_ > node->outSecond_))
            {
                PDEBUG("node->outSecond_ %ld nowSecond_ %ld", node->outSecond_, nowSecond_);
                OrderNodePtr cmdNode = node->cmdOrd_;
                cmdNode->cmdRet_ = CMD_OUTTIME_CODE;
                common::CmdResultQueue::instance().insertCmdResult(cmdNode);
            }else
            {
                if(node->cli_)
                {
                    if(node->cmdOrd_ == nullptr)
                    {
                        if(node->cli_->tcpCliState() == REDIS_CLIENT_STATE_INIT)
                        {
                            ioAddRedisClient(node->cli_);
                            int ret = node->cli_->connectServer(evbase);
                            if(ret)
                            {
                                CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisSvrOnConnect(node->cli_->redisMgrfd(), CONNECT_REDISVR_RESET, node->cli_->redisSvrIpaddr(), node->cli_->redisSvrPort());
                            }
                        }else{
                            node->cli_->disConnect(true);//TODO
                        }
                    }else{
                        if(node->cli_->tcpCliState() == REDIS_CLIENT_STATE_INIT)
                        {
                            OrderNodePtr cmdNode = node->cmdOrd_;
                            cmdNode->cmdRet_ = CMD_SVR_CLOSER_CODE;
                            common::CmdResultQueue::instance().insertCmdResult(cmdNode);
                        }else{
                            node->cli_->requestCmd(node->cmdOrd_);
                        }
                    }
                }else{
                    PERROR("TODO");
                }
            }
        }else{
            break;
        }
    }
    uint64_t num = one % 2;
    if(num)
    {
        event_base_loopbreak(evbase);
        return;
    }

    if(lastSecond_ == 0)
    {
        lastSecond_ = nowSecond_;
    }else{
        if(nowSecond_ != lastSecond_)
        {
            for(size_t i = 0; i < ioRedisClients_.size(); i++)
            {
                if(ioRedisClients_[i])
                {
                    if(ioRedisClients_[i]->releaseState())
                    {
                        ioRedisClients_[i]->disConnect(true);//所有的rediscli析构都要在这里析构，否则会有野指针，空指针
                        ioDisRedisClient(ioRedisClients_[i]);
                    }else{
                        ioRedisClients_[i]->checkOutSecondCmd(nowSecond_);
                    }
                }
            }
            lastSecond_ = nowSecond_;
        }
    }
}

}

