#ifndef __REDIS_CLIENT_ASYNC_H__
#define __REDIS_CLIENT_ASYNC_H__

#include "vector"

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"

#include <event.h>

#include <async.h>

#include "MutexLock.h"
#include "OrderInfo.h"

#include "Atomic.h"
#include "OrderInfo.h"
#include "RedisCommon.h"
#include "RedisSocket.h"

#include "../RedisAsync.h"

namespace REDIS_ASYNC_CLIENT
{

/*
 * [function]
 * @author
 * @param
 * @param
 * @param
 *
 * @return
 */
class RedisCliOrderNode
{
public:
    RedisCliOrderNode(const REDIS_ASYNC_CLIENT::RedisSocketPtr& cli, uint64_t outsecond = 0)
        :outSecond_(outsecond)
        ,cli_(cli)
        ,cmdOrd_(nullptr)
    {
    }

    RedisCliOrderNode(const REDIS_ASYNC_CLIENT::RedisSocketPtr& cli, const common::OrderNodePtr& cmd, uint64_t outsecond = 0)
        :outSecond_(outsecond)
        ,cli_(cli)
        ,cmdOrd_(cmd)
    {
    }

    RedisCliOrderNode(const RedisCliOrderNode& that)
        :outSecond_(0)
        ,cli_(nullptr)
        ,cmdOrd_(nullptr)
    {
        *this = that;
    }

    RedisCliOrderNode& operator=(const RedisCliOrderNode& that)
    {
        if(this == &that) return *this;

        outSecond_ = that.outSecond_;
        cli_ = that.cli_;
        cmdOrd_ = that.cmdOrd_;
        
        return *this;
    }
    
    ~RedisCliOrderNode()
    {
        if(cli_)
        {
            cli_.reset();
        }
        if(cmdOrd_)
        {
            cmdOrd_.reset();
        }
    }

    uint64_t outSecond_;// 超时时间，精确到秒钟
    RedisSocketPtr cli_;
    common::OrderNodePtr cmdOrd_;//如果 cmdOrd_ 是空指针，则表示要建立连接或者关闭连接，根据内部标志位判断
};

typedef std::shared_ptr<RedisCliOrderNode> RedisCliOrderNodePtr;
typedef std::deque<RedisCliOrderNodePtr> DequeRedisCliOrderNodePtr;
typedef std::list<RedisCliOrderNodePtr> ListRedisCliOrderNodePtr;

/*
 * [OrderNodeDeque] 请求队列，每个节点上有一个请求队列，队列可以控制最大等待大小
 * @author
 * @param
 * @param
 * @param
 *
 * @return
 */
class OrderNodeDeque
{
public:
    OrderNodeDeque()
        :mutex_()
        ,queue_()
    {
        PDEBUG("OrderNodeDeque init");
    }

    ~OrderNodeDeque()
    {
        PERROR("~OrderNodeDeque exit");
    }

    void orderNodeInsert(const RedisCliOrderNodePtr& node)
    {
        SafeMutexLock lock(mutex_);
        queue_.push_back(node);
    }

    RedisCliOrderNodePtr dealOrderNode()
    {
        SafeMutexLock lock(mutex_);
        RedisCliOrderNodePtr node(nullptr);
        if(!queue_.empty())
        {
            node = queue_.front();
            queue_.pop_front();
        }
        return node;
    }

private:
    common::MutexLock mutex_;
    DequeRedisCliOrderNodePtr queue_;
};

class RedisClient
{
public:
    RedisClient(const RedisNodeAddInfoPtr& clientInfo, const RedisInitParPtr& initPar);

    ~RedisClient();

    void requestCmd(const common::OrderNodePtr& order, uint64_t nowSecond);

    void checkOutSecondCmd(uint64_t nowSecond);
 
private:

    RedisNodeAddInfoPtr clientAddrInfo_;
    RedisInitParPtr redisInitPar_;
    VectRedisSocketPtr masterSockets_;
    VectRedisSocketPtr slaveSockets_;
    OrderNodeDeque orderDeque_;
};

typedef std::shared_ptr<RedisClient> RedisClientPtr;
typedef std::vector<RedisClientPtr> VectRedisClientPtr;

}
#endif // end __REDIS_CLIENT_ASYNC_H__
