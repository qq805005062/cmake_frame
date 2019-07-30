#ifndef __REDIS_SLIDE_SIZE_H__
#define __REDIS_SLIDE_SIZE_H__

#include <stdint.h>
#include <memory>
#include <vector>
#include <semaphore.h>
#include <mutex>

#include "Incommon.h"
#include "OrderInfo.h"

//#include "RedisSlide.h"
/*
 *redis 服务端是单线程，所以服务处理的顺序一定是先到先处理。
 *另外redis协议中是没有seq这个概念的。之前做过但连接异步测试，基本上所有请求都回来的。所以可以认为服务端是支持完全异步
 *
 *
 *
 */
class RedisReqNode
{
public:
    RedisReqNode()
        :reqSeq(0)
        ,orderNode(nullptr)
    {
    }

    ~RedisReqNode()
    {
        if(orderNode)
        {
            orderNode.reset();
        }
    }

    RedisReqNode(const RedisReqNode& that)
    {
        *this = that;
    }

    RedisReqNode& operator=(const RedisReqNode& that)
    {
        if(this == &that) return *this;

        reqSeq = that.reqSeq;
        orderNode = that.orderNode;

        return *this;
    }

    uint32_t reqSeq;
    common::OrderNodePtr orderNode;
};

typedef std::shared_ptr<RedisReqNode> RedisReqNodePtr;
typedef std::vector<RedisReqNodePtr> VectRedisReqNodePtr;

class RedisSlide
{
public:
    RedisSlide(unsigned int size)
    {
    }

    ~RedisSlide()
    {
    }

private:

   size_t pushIndex_;
   size_t popIndex_;
   
   unsigned int slideSize_;
   volatile unsigned int freeSize_;
    
    VectRedisReqNodePtr slideRedisWindow_;

    std::mutex mutex_;
    sem_t freeSem_;

};

#endif
