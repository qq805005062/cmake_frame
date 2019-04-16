#ifndef __COMMON_INCOMMON_H__
#define __COMMON_INCOMMON_H__

#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <stdlib.h>
#include <string>
#include <memory>
#include <vector>
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_IP_ADDR_LEN                         15
#define MAX_PORT_NUM_LEN                        5

#define PDEBUG(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class RedisSvrInfo
{
public:

    RedisSvrInfo()
        :port_(0)
        ,ipAddr_()
    {
    }

    RedisSvrInfo(int p, const std::string& ip)
        :port_(p)
        ,ipAddr_(ip)
    {
    }

    RedisSvrInfo(const std::string& ip, int p)
        :port_(p)
        ,ipAddr_(ip)
    {
    }

    RedisSvrInfo(const RedisSvrInfo& that)
        :port_(0)
        ,ipAddr_()
    {
        *this = that;
    }

    RedisSvrInfo& operator=(const RedisSvrInfo& that)
    {
        if(this == &that) return *this;

        port_ = that.port_;
        ipAddr_ = that.ipAddr_;
        
        return *this;
    }

    int port_;
    std::string ipAddr_;
};

typedef std::shared_ptr<RedisSvrInfo> RedisSvrInfoPtr;
typedef std::vector<RedisSvrInfoPtr> RedisSvrInfoPtrVect;

class RedisClusterNode
{
public:
    RedisClusterNode()
        :slotStart(0)
        ,slotEnd(0)
        ,master_(nullptr)
        ,slave_()
    {
    }

    RedisClusterNode(const RedisClusterNode& that)
        :slotStart(0)
        ,slotEnd(0)
        ,master_(nullptr)
        ,slave_()
    {
        *this = that;
    }

    RedisClusterNode& operator=(const RedisClusterNode& that)
    {
        if(this == &that) return *this;

        slotStart = that.slotStart;
        slotEnd = that.slotEnd;
        master_ = that.master_;
        slave_ = that.slave_;

        return *this;
    }

    uint16_t slotStart;
    uint16_t slotEnd;
    RedisSvrInfoPtr master_;
    RedisSvrInfoPtrVect slave_;
};

typedef std::shared_ptr<RedisClusterNode> RedisClusterNodePtr;
typedef std::vector<RedisClusterNodePtr> RedisClusterNodePtrVect;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * [utilFristchar] 字符串中查找字符第一次出现的指针位置
 * @author xiaoxiao 2019-04-02
 * @param str 需要查询出现字符的字符串头指针
 * @param c 需要查找的字符
 *
 * @return 第一个字符的在字符串中的字符位置，没有找到为NULL
 */
inline char* utilFristchar(char *str,const char c)
{
    char *p = str;
    if(!str)
    {
        return NULL;
    }
    while(*p)
    {
        if(*p == c)
        {
            return p;
        }
        else
        {
            p++;
        }
    }
    return NULL;
}

/*
 * [utilFristchar] 字符串中查找字符第一次出现的指针位置
 * @author xiaoxiao 2019-04-02
 * @param str 需要查询出现字符的字符串头指针
 * @param c 需要查找的字符
 *
 * @return 第一个字符的在字符串中的字符位置，没有找到为NULL
 */
inline const char* utilFristConstchar(const char *str,const char c)
{
    const char *p = str;
    if(!str)
    {
        return NULL;
    }
    while(*p)
    {
        if(*p == c)
        {
            return p;
        }
        else
        {
            p++;
        }
    }
    return NULL;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
