#ifndef __COMMON_ORDERINFO_H__
#define __COMMON_ORDERINFO_H__

//#include "OrderInfo.h"

#include <deque>

#include "MutexLock.h"
#include "RedisClient.h"

namespace common
{

class OrderNode
{
public:
    OrderNode()
        :cmdPri(nullptr)
        ,cmdMsg()
        ,cli(nullptr)
    {
    }

    OrderNode(const REDIS_ASYNC_CLIENT::RedisClientPtr& cl)
        :cmdPri(nullptr)
        ,cmdMsg()
        ,cli(cl)
    {
    }

    OrderNode(const REDIS_ASYNC_CLIENT::RedisClientPtr& cl, const std::string& msg, void* pri = NULL)
        :cmdPri(pri)
        ,cmdMsg(msg)
        ,cli(cl)
    {
    }

    OrderNode(const REDIS_ASYNC_CLIENT::RedisClientPtr& cl, const char* msg, size_t msgLen, void* pri = NULL)
        :cmdPri(pri)
        ,cmdMsg(msg, msgLen)
        ,cli(cl)
    {
    }

    OrderNode(const OrderNode& that)
        :cmdPri(nullptr)
        ,cmdMsg()
        ,cli(nullptr)
    {
        *this = that;
    }

    OrderNode& operator=(const OrderNode& that)
    {
        if(this == &that) return *this;

        cmdPri = that.cmdPri;
        cmdMsg = that.cmdMsg;
        cli = that.cli;
    }

    ~OrderNode()
    {
    }

    void setOrderNodeCmdmsg(const std::string& msg)
    {
        cmdMsg.assign(msg);
    }

    void setOrderNodeCmdmsg(const char* msg, size_t msglen)
    {
        cmdMsg.assign(msg, msglen);
    }

    void setOrderNodeRediscli(const REDIS_ASYNC_CLIENT::RedisClientPtr& cl)
    {
        cli = cl;
    }

    void setOrderNodeCmdPri(void* pri)
    {
        cmdPri = pri;
    }

    std::string orderNodeCmdmsg()
    {
        return cmdMsg;
    }

    REDIS_ASYNC_CLIENT::RedisClientPtr orderNodeRediscli()
    {
        return cli;
    }

    void* orderNodePri()
    {
        return cmdPri;
    }

private:
    void* cmdPri;
    std::string cmdMsg;
    REDIS_ASYNC_CLIENT::RedisClientPtr cli;//如果cmdMsg 是空，则表示要建立连接或者关闭连接，根据内部标志位判断
};

typedef std::shared_ptr<OrderNode> OrderNodePtr;
typedef std::deque<OrderNodePtr> OrderNodePtrDeque;

class OrderNodeDeque
{
public:
    OrderNodeDeque()
        :mutex_()
        ,queue_()
    {
    }

    ~OrderNodeDeque()
    {
    }

    void orderNodeInsert(const OrderNodePtr& node)
    {
        SafeMutexLock lock(mutex_);
        queue_.push_back(node);
    }

    OrderNodePtr dealOrderNode()
    {
        SafeMutexLock lock(mutex_);
        OrderNodePtr node(nullptr);
        if(!queue_.empty())
        {
            node = queue_.front();
            queue_.pop_front();
        }
        return node;
    }

private:
    MutexLock mutex_;
    OrderNodePtrDeque queue_;
};

}//end namespace common

#endif//end __COMMON_ORDERINFO_H__

