#ifndef __COMMON_LIBEVENT_IO_H__
#define __COMMON_LIBEVENT_IO_H__

#include <vector>
#include <list>

#include <event2/event.h>
#include <event2/event_struct.h>

#include "MutexLock.h"

#include "Singleton.h"
#include "noncopyable.h"

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
    RedisCliOrderNode(const REDIS_ASYNC_CLIENT::RedisClientPtr& cli, uint64_t outsecond = 0)
        :outSecond_(outsecond)
        ,cli_(cli)
        ,cmdOrd_(nullptr)
    {
    }

    RedisCliOrderNode(const REDIS_ASYNC_CLIENT::RedisClientPtr& cli, const common::OrderNodePtr& cmd, uint64_t outsecond = 0)
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
    REDIS_ASYNC_CLIENT::RedisClientPtr cli_;
    OrderNodePtr cmdOrd_;//如果 cmdOrd_ 是空指针，则表示要建立连接或者关闭连接，根据内部标志位判断
};

typedef std::shared_ptr<RedisCliOrderNode> RedisCliOrderNodePtr;
typedef std::deque<RedisCliOrderNodePtr> DequeRedisCliOrderNodePtr;
typedef std::list<RedisCliOrderNodePtr> ListRedisCliOrderNodePtr;
/*
 * [TimerOrderDeque] 定时命令队列，此队列是存放需要延迟定时执行的命令。所以内部的超时时间为定时执行的时间。并且实际的命令不可以有超时时间
 * @author xiaoxiao 2019-05-17
 * @param
 * @param
 * @param
 *
 * @return
 */
class TimerOrderDeque : public common::noncopyable
{
public:
    TimerOrderDeque()
        :mutex_()
        ,list_()
    {
    }

    ~TimerOrderDeque()
    {
        ListRedisCliOrderNodePtr ().swap(list_);
    }

    static TimerOrderDeque& instance() { return common::Singleton<TimerOrderDeque>::instance(); }

    void insertTimerOrder(const RedisCliOrderNodePtr& node)
    {
        SafeMutexLock lock(mutex_);
        if(list_.empty())
        {
            list_.push_front(node);
            return;
        }
        if(node->outSecond_ == 0)
        {
            list_.push_front(node);
            return;
        }
        for(ListRedisCliOrderNodePtr::iterator iter = list_.begin(); iter != list_.end(); iter++)
        {
            if(node->outSecond_ <= (*iter)->outSecond_ )
            {
                list_.insert(iter, node);
                return;
            }
        }
        list_.push_back(node);
        return;
    }

    RedisCliOrderNodePtr dealTimerOrder(uint64_t nowSecond)
    {
        SafeMutexLock lock(mutex_);

        for(ListRedisCliOrderNodePtr::iterator iter = list_.begin(); iter != list_.end(); iter++)
        {
            if(((*iter)->outSecond_ == 0) || (nowSecond >= (*iter)->outSecond_))
            {
                RedisCliOrderNodePtr node = (*iter);
                list_.pop_front();
                return node;
            }else{
                return RedisCliOrderNodePtr();
            }
        }

        return RedisCliOrderNodePtr();
    }
    
private:
    MutexLock mutex_;
    ListRedisCliOrderNodePtr list_;

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

    int libeventIoOrder(const RedisCliOrderNodePtr& node, uint64_t nowsecond);

    int libeventIoWakeup(uint64_t nowsecond);

    void ioDisRedisClient(const REDIS_ASYNC_CLIENT::RedisClientPtr& cli);

    void ioAddRedisClient(const REDIS_ASYNC_CLIENT::RedisClientPtr& cli);

////////////////////////////////////////////////////////////////////////////////////
    void handleRead();
private:

    int wakeupFd;

    struct event_base *evbase;

    volatile uint64_t nowSecond_;
    volatile uint64_t lastSecond_;
    
    struct event wake_event;
    OrderNodeDeque orderDeque_;
    REDIS_ASYNC_CLIENT::VectRedisClientPtr ioRedisClients_;
};

typedef std::shared_ptr<LibeventIo> LibeventIoPtr;

}//end namespace common
#endif //end __COMMON_LIBEVENT_IO_H__
