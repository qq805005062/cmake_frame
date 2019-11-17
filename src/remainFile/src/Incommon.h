#ifndef __REMAIN_MGR_INCOMMON_H__
#define __REMAIN_MGR_INCOMMON_H__

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <time.h>
#include <sys/time.h>

#include <string>
#include <memory>

#define DEBUG(fmt, args...)     fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define INFO(fmt, args...)      fprintf(stderr, "%s :: %s() %d: INFO " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define WARN(fmt, args...)      fprintf(stderr, "%s :: %s() %d: WARN " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ERROR(fmt, args...)     fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

const static std::string reportDir = "RemainRe", reportBakDir = "RemainReBak", moDir = "RemainMo", moBakDir = "RemainMoBak", mtDir = "RemainMt", mtBak = "RemainMtBak";
const static char tailOkFile[] = ".ok", tailBakFile[] = ".bak", tailTmpFile[] = ".tmp";

#define TAIL_OKFILE_STRLEN      3

#define RE_REMAIN_FLAG          1
#define RE_REMAIN_BAK_FLAG      2
#define MO_REMAIN_FLAG          3
#define MO_REMAIN_BAK_FLAG      4
#define MT_REMAIN_FLAG          5
#define MT_REMAIN_BAK_FLAG      6

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

inline int accessPath(const char *muldir)
{
    size_t len = 0;
    char str[1024] = {0};

    strncpy(str, muldir, 1024);
    len= strlen(str);

    if( len > 0 && access(str, F_OK|W_OK|R_OK) != 0 )
    {
        return -1;
    }
    return 0;
}

inline int renameFile(const char* oldName, const char* newName)
{
    return ::rename(oldName, newName);
}

inline int mkdirs(const char *muldir) 
{
    size_t i = 0, len = 0;
    char str[1024] = {0};

    strncpy(str, muldir, 1024);
    len= strlen(str);
    for(i=1; i<len; i++)
    {
        if( str[i]=='/' )
        {
            str[i] = '\0';
            if(access(str, F_OK) != 0 )
            {
                if(mkdir(str, 0755 ) != 0)
                {
                    fprintf(stderr, "%s :: %s() %d: ERROR mkdir %s error %s \n", __FILE__, __FUNCTION__, __LINE__, str, strerror(errno));
                    return -1;
                }
            }
            str[i]='/';
        }
    }

    if( len > 0 && access(str, F_OK) != 0 )
    {
        if(mkdir(str, 0755 ) != 0)
        {
            fprintf(stderr, "%s :: %s() %d: ERROR mkdir %s error %s \n", __FILE__, __FUNCTION__, __LINE__, str, strerror(errno));
            return -1;
        }
    }

    return 0;
}

inline char *endCharStr(char *buff)
{
    char *p = buff;
    while(*p!=0)
    {
        p++;
    }
    return p;
}

inline const char *endConstCharStr(const char *buff)
{
    const char *p = buff;
    while(*p!=0)
    {
        p++;
    }
    return p;
}

inline int strIntLen(const char *str)
{
    int len = 0;
    if(!str)
    {
        return len;
    }
    while(*str)
    {
        str++;
        len++;
    }
    return len;
}

inline char* strFristChar(char *str, char c)
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

inline const char* strFristConstChar(const char *str, char c)
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

inline const char* strFirstConstStr(const char *str, const char *tar)
{
    const char *q = tar;
    const char tar_fri = *tar;
    const char *p = str,*p_for = NULL;
    int p_len = 0,q_len = 0,i;

    if(!str)
    {
        return NULL;
    }
    p_len = strIntLen(p);
    q_len = strIntLen(q);
    if(q_len > p_len)
    {
        return NULL;
    }

    do{
        p_for = strFristConstChar(p,tar_fri);
        if(!p_for)
        {
            return NULL;
        }
        else
        {
            p = p_for;
        }
        p_len = strIntLen(p);
        if(q_len > p_len)
        {
            return NULL;
        }
        for(i = 0;i < q_len;i++)
        {
            if(*p_for == *q)
            {
                p_for++;
                q++;
                continue;
            }else{
                break;
            }
        }
        if(i == q_len)
        {
            return p;
        }
        else
        {
            p++;
            q = tar;
        }
    }while(1);

    return NULL;
}

inline std::string locakExePath()
{
    std::string result;
    char buf[1024];
    ssize_t n = ::readlink("/proc/self/exe", buf, sizeof buf);
    if (n > 0)
    {
        result.assign(buf, n);
        size_t pos = result.find_last_of("/");
        if (pos != std::string::npos)
        {
            result.assign(buf, pos);
        }
    }
    return result;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"

//将int64_t的数值转换成8个字节的内存数值
inline void TranMsgIdI64ToChar(unsigned char* MsgId, int64_t IMsgId)
{
    int64_t j = IMsgId;

    MsgId[0] = (unsigned char)((j >> 56) & 0xff);
    MsgId[1] = (unsigned char)((j >> 48) & 0xff);
    MsgId[2] = (unsigned char)((j >> 40) & 0xff);
    MsgId[3] = (unsigned char)((j >> 32) & 0xff);
    MsgId[4] = (unsigned char)((j >> 24) & 0xff);
    MsgId[5] = (unsigned char)((j >> 16) & 0xff);
    MsgId[6] = (unsigned char)((j >> 8) & 0xff);
    MsgId[7] = (unsigned char)(j & 0xff);

    return;
}

//将8个字节内存的数值转换为int64_t的数值
inline int64_t TranMsgIdCharToI64(const unsigned char *szMsgId)
{
    if (NULL == szMsgId)
    {
        return 0;
    }

    int64_t j = 0;

    const unsigned char* p = szMsgId;
    j += (int64_t)(*p)     << 56;
    j += (int64_t)*(p + 1) << 48;
    j += (int64_t)*(p + 2) << 40;
    j += (int64_t)*(p + 3) << 32;
    j += (int64_t)*(p + 4) << 24;
    j += (int64_t)*(p + 5) << 16;
    j += (int64_t)*(p + 6) << 8;
    j += (int64_t)*(p + 7);

    return j;
}

#endif
