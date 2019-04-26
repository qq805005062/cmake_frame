#ifndef __COMMON_LIBEVENT_IO_H__
#define __COMMON_LIBEVENT_IO_H__

#include <vector>

#include <event2/event.h>
#include <event2/event_struct.h>

#include "OrderInfo.h"
#include "RedisClient.h"

namespace common
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
    RedisCliOrderNode(const REDIS_ASYNC_CLIENT::RedisClientPtr& cli, const common::OrderNodePtr& cmd = nullptr)
        :cmdOrd_(cmd)
        ,cli_(cli)
    {
    }

    RedisCliOrderNode(const RedisCliOrderNode& that)
        :cmdOrd_(nullptr)
        ,cli_(nullptr)
    {
        *this = that;
    }

    RedisCliOrderNode& operator=(const RedisCliOrderNode& that)
    {
        if(this == &that) return *this;

        cmdOrd_ = that.cmdOrd_;
        cli_ = that.cli_;

        return *this;
    }
    
    ~RedisCliOrderNode()
    {
    }

    OrderNodePtr cmdOrd_;//如果 cmdOrd_ 是空指针，则表示要建立连接或者关闭连接，根据内部标志位判断
    REDIS_ASYNC_CLIENT::RedisClientPtr cli_;
};

typedef std::shared_ptr<RedisCliOrderNode> RedisCliOrderNodePtr;
typedef std::deque<RedisCliOrderNodePtr> DequeRedisCliOrderNodePtr;

/*
 * [function]
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
    }

    ~OrderNodeDeque()
    {
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
    MutexLock mutex_;
    DequeRedisCliOrderNodePtr queue_;
};

/*
 * [function]
 * @author
 * @param
 * @param
 * @param
 *
 * @return
 */
class LibeventIo
{
public:

    LibeventIo();

    ~LibeventIo();

    int libeventIoReady();

    int libeventIoExit();

    int libeventIoOrder(const RedisCliOrderNodePtr& node);

    void ioDisRedisClient(const REDIS_ASYNC_CLIENT::RedisClientPtr& cli);

    void ioAddRedisClient(const REDIS_ASYNC_CLIENT::RedisClientPtr& cli);

////////////////////////////////////////////////////////////////////////////////////
    void handleRead();
private:

    int libeventIoWakeup();

    int wakeupFd;

    struct event_base *evbase;
    
    uint64_t lastSecond_;
    
    struct event wake_event;
    OrderNodeDeque orderDeque_;
    REDIS_ASYNC_CLIENT::VectRedisClientPtr ioRedisClients_;
};

typedef std::shared_ptr<LibeventIo> LibeventIoPtr;

}//end namespace common
#endif //end __COMMON_LIBEVENT_IO_H__
