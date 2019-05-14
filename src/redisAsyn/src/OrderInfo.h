#ifndef __COMMON_ORDERINFO_H__
#define __COMMON_ORDERINFO_H__

//#include "OrderInfo.h"
#include <map>
#include <deque>

#include "Singleton.h"
#include "noncopyable.h"

#include "MutexLock.h"
#include "Condition.h"

#include "../RedisAsync.h"

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
class OrderNode
{
public:
    OrderNode(const std::string& msg, int outSecond = 0)
        :cmdRet_(0)
        ,cmdOutSecond_(outSecond)
        ,cmdQuerySecond_(0)
        ,cmdPri_(nullptr)
        ,cmdMsg_(msg)
        ,resultCb_(nullptr)
        ,cmdResult_()
    {
    }

    OrderNode(const std::string& msg, int outSecond = 3, const CLUSTER_REDIS_ASYNC::CmdResultCallback& cb = NULL, void* pri = NULL)
        :cmdRet_(0)
        ,cmdOutSecond_(outSecond)
        ,cmdQuerySecond_(0)
        ,cmdPri_(pri)
        ,cmdMsg_(msg)
        ,resultCb_(cb)
        ,cmdResult_()
    {
    }

    OrderNode(const OrderNode& that)
        :cmdRet_(0)
        ,cmdOutSecond_(0)
        ,cmdQuerySecond_(0)
        ,cmdPri_(nullptr)
        ,cmdMsg_()
        ,resultCb_(nullptr)
        ,cmdResult_()
    {
        *this = that;
    }

    OrderNode& operator=(const OrderNode& that)
    {
        if(this == &that) return *this;

        cmdRet_ = that.cmdRet_;
        cmdOutSecond_ = that.cmdOutSecond_;
        cmdQuerySecond_ = that.cmdQuerySecond_;
        cmdPri_ = that.cmdPri_;
        cmdMsg_ = that.cmdMsg_;
        resultCb_ = that.resultCb_;
        cmdResult_ = that.cmdResult_;

        return *this;
    }

    ~OrderNode()
    {
    }

    
    int cmdRet_;//查询命令执行结果
    int cmdOutSecond_;//超时时间，单位秒钟
    uint64_t cmdQuerySecond_;//命令执行秒钟

    void* cmdPri_;//私有指针
    std::string cmdMsg_;//命令的字符串

    CLUSTER_REDIS_ASYNC::CmdResultCallback resultCb_;//命令直接结果的回调
    CLUSTER_REDIS_ASYNC::StdVectorStringPtr cmdResult_;//执行结果的string vect
};

typedef std::shared_ptr<OrderNode> OrderNodePtr;
typedef std::deque<OrderNodePtr> OrderNodePtrDeque;
typedef std::map<uint32_t, OrderNodePtr> SeqOrderNodeMap;

/*
 * [function]
 * @author
 * @param
 * @param
 * @param
 *
 * @return
 */
#if 1
class CmdResultQueue : public common::noncopyable
{
public:
    CmdResultQueue()
        :mutex_()
        ,queue_()
    {
    }

    ~CmdResultQueue()
    {
    }
    
    static CmdResultQueue& instance() { return common::Singleton<CmdResultQueue>::instance(); }

    void insertCmdResult(const OrderNodePtr& node)
    {
        {
            SafeMutexLock lock(mutex_);
            queue_.push_back(node);
        }
        CLUSTER_REDIS_ASYNC::RedisAsync::instance().asyncCmdResultCallBack();
    }

    OrderNodePtr takeCmdResult()
    {
        SafeMutexLock lock(mutex_);
        if(queue_.empty())
        {
            return OrderNodePtr();
        }

        OrderNodePtr node = queue_.front();
        queue_.pop_front();
        return node;
    }

private:

    MutexLock mutex_;
    OrderNodePtrDeque queue_;
};
#else
class CmdResultQueue : public common::noncopyable
{
public:
    CmdResultQueue()
        :isExit_(0)
        ,mutex_()
        ,notEmpty_(mutex_)
        ,queue_()
    {
    }

    ~CmdResultQueue()
    {
    }
    
    static CmdResultQueue& instance() { return common::Singleton<CmdResultQueue>::instance(); }

    void cmdResuleExit()
    {
        isExit_ = 1;
        notEmpty_.notifyAll();
    }

    void insertCmdResult(const OrderNodePtr& node)
    {
        SafeMutexLock lock(mutex_);
        queue_.push_back(node);
        notEmpty_.notify();
        CLUSTER_REDIS_ASYNC::RedisAsync::instance().asyncCmdResultCallBack();
    }

    OrderNodePtr takeCmdResult()
    {
        SafeMutexLock lock(mutex_);
        while(queue_.empty())
        {
            if(isExit_)
            {
                return OrderNodePtr();
            }
            notEmpty_.wait();
        }

        OrderNodePtr node = queue_.front();
        queue_.pop_front();
        return node;
    }

private:
    int isExit_;
    MutexLock mutex_;
    Condition notEmpty_;
    OrderNodePtrDeque queue_;
};
#endif
}//end namespace common

#endif//end __COMMON_ORDERINFO_H__

