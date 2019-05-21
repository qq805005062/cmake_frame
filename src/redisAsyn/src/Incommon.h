#ifndef __COMMON_INCOMMON_H__
#define __COMMON_INCOMMON_H__

#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <memory>
#include <vector>

#include <time.h>
#include <sys/time.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_IP_ADDR_LEN                         15
#define MAX_PORT_NUM_LEN                        5

#define PTRACE(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: TRACE " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PDEBUG(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFAULT_CMD_OUTSECOND                   3


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void split(const std::string& str, const std::string& delim, std::vector<std::string>& result);

uint16_t getKeySlotIndex(const std::string& key);

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

inline int64_t microSecondSinceEpoch(int64_t* second = NULL)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if(second)
    {
        *second = tv.tv_sec;
    }
    int64_t microSeconds = tv.tv_sec * 1000000 + tv.tv_usec;
    return microSeconds;
}

inline int64_t secondSinceEpoch()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return seconds;
}


#endif
