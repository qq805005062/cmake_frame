#ifndef __REDIS_COMMON_H__
#define __REDIS_COMMON_H__

#include <memory>

//#include "RedisCommon.h"
namespace REDIS_ASYNC_CLIENT
{

class RedisSvrAddInfo
{
public:
    RedisSvrAddInfo(int p, const std::string& ip)
        :port_(p)
        ,ipAddr_(ip)
    {
    }

    RedisSvrAddInfo(const std::string& ip, int p)
        :port_(p)
        ,ipAddr_(ip)
    {
    }

    ~RedisSvrAddInfo()
    {
    }

    RedisSvrAddInfo(const RedisSvrAddInfo& that)
        :port_(0)
        ,ipAddr_()
    {
        *this = that;
    }

    RedisSvrAddInfo& operator=(const RedisSvrAddInfo& that)
    {
        if(this == &that) return *this;

        port_ = that.port_;
        ipAddr_ = that.ipAddr_;
        
        return *this;
    }

    bool operator ==(const RedisSvrAddInfo& that)
    {
        if(this == &that)
        {
            return true;
        }
        
        if((port_ == that.port_) && (ipAddr_.compare(that.ipAddr_) == 0))
        {
            return true;
        }

        return false;
    }

    int port_;
    std::string ipAddr_;
};

typedef std::shared_ptr<RedisSvrAddInfo> RedisSvrAddInfoPtr;
typedef std::vector<RedisSvrAddInfoPtr> VectRedisSvrAddInfoPtr;

class RedisNodeAddInfo
{
public:
    RedisNodeAddInfo()
        :mastr_()
        ,slave_()
    {
    }

    RedisNodeAddInfo(const RedisNodeAddInfo& that)
        :mastr_()
        ,slave_()
    {
        *this = that;
    }

    RedisNodeAddInfo& operator=(const RedisNodeAddInfo& that)
    {
        if(this = &that) return *this;
        
        mastr_ = that.mastr_;
        slave_ = that.slave_;

        return *this;
    }

    
    
    ~RedisNodeAddInfo()
    {
        if(mastr_)
        {
            mastr_.reset();
        }

        if(slave_)
        {
            slave_.reset();
        }
    }

    RedisSvrAddInfoPtr mastr_;
    RedisSvrAddInfoPtr slave_;
};

typedef std::shared_ptr<RedisNodeAddInfo> RedisNodeAddInfoPtr;


class RedisInitPar
{
public:
    RedisInitPar()
        :minConnNum_(1)
        ,maxConnNum_(1)
        ,connSlideSize_(10)
        ,maxWaitQueSize_(minConnNum_ * connSlideSize_)
        ,keepSecond_(10)
        ,connOutSecond_(3)
    {
    }

    RedisInitPar(const RedisInitPar& that)
        :minConnNum_(1)
        ,maxConnNum_(1)
        ,connSlideSize_(10)
        ,maxWaitQueSize_(minConnNum_ * connSlideSize_)
        ,keepSecond_(10)
        ,connOutSecond_(3)
    {
        *this = that;
    }

    RedisInitPar& operator=(const RedisInitPar& that)
    {
        if(this == &that) return *this;

        minConnNum_ = that.minConnNum_;
        maxConnNum_ = that.maxConnNum_;
        connSlideSize_ = that.connSlideSize_;
        maxWaitQueSize_ = that.maxWaitQueSize_;
        keepSecond_ = that.keepSecond_;
        connOutSecond_ = that.connOutSecond_;

        return *this;
    }

    ~RedisInitPar()
    {
    }

    int minConnNum_;//最小连接数
    int maxConnNum_;//最大连接数
    int connSlideSize_;//每个连接上的滑动窗口大小，这个是针对对每个连接的，滑动在每个连接上，等待队列是同一个服务端所有连接共用的
    int maxWaitQueSize_;//最大等待队列大小，这个是针对每一个redis节点的配置，可以都多个连接，但是共用一个异步请求队列，主从算一个节点
    int keepSecond_;//保持心跳间隔秒钟
    int connOutSecond_;//连接超时秒钟
};
typedef std::shared_ptr<RedisInitPar> RedisInitParPtr;

}

#endif
