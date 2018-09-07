#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common/Base64.h"

namespace common
{

static const char* base64Code = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char Base64::Base2Chr(unsigned char n)
{
    n &= 0x3F;
    if (n < 26)
        return static_cast<char>(n + 'A');
    else if (n < 52)
        return static_cast<char>(n - 26 + 'a');
    else if (n < 62)
        return static_cast<char>(n - 52 + '0');
    else if (n == 62)
        return '+';
    else
        return '/';
}

unsigned char Base64::Chr2Base(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return static_cast<unsigned char>(c - 'A');
    }
    else if (c >= 'a' && c <= 'z')
    {
        return static_cast<unsigned char>(c - 'a' + 26);
    }
    else if (c >= '0' && c <= '9')
    {
        return static_cast<unsigned char>(c - '0' + 52);
    }
    else if (c == '+')
    {
        return 62;
    }
    else if (c == '/')
    {
        return 63;
    }
    else
    {
        return 64;
    }
}

bool Base64::Encode(const std::string& src, std::string* dst)
{
    if (0 == src.size() || nullptr == dst)
        return false;

    dst->resize(Base64EncodeLen(src.size()));

    unsigned int c = -1;

    unsigned char* p = reinterpret_cast<unsigned char* >(&(*dst)[0]);
    unsigned char* s = p;
    unsigned char* q = reinterpret_cast<unsigned char* >(const_cast<char* >(&src[0]));

    for (size_t i = 0; i < src.size(); )
    {
        // 处理的时候，都是把24bit当做一个单位，因为3*8 = 4*6
        c = static_cast<unsigned int>(q[i++]);
        c *= 256;
        if (i < src.size())
            c += static_cast<unsigned int>(q[i]);
        i++;
        c *= 256;
        if (i < src.size())
            c += static_cast<unsigned int>(q[i]);
        i++;

        // 每次取6bit当作一个8bit的char放入p中
        p[0] = base64Code[(c & 0x00fc0000) >> 18];
        p[1] = base64Code[(c & 0x0003f000) >> 12];
        p[2] = base64Code[(c & 0x00000fc0) >> 6];
        p[3] = base64Code[(c & 0x0000003f) >> 0];

        // 这里是处理结尾情况
        if (i > src.size())
            p[3] = '=';

        if (i > src.size() + 1)
            p[2] = '=';

        p += 4; // 编码后的数据指针相应的移动
    }

    *p = 0; // 防止野指针
    dst->resize(p - s);

    return true;
}

bool Base64::Decode(const std::string& src, std::string* dst)
{
    if (0 == src.size() || nullptr == dst)
    {
        return false;
    }

    dst->resize(Base64DecodeLen(src.size()));

    unsigned char* p = reinterpret_cast<unsigned char* >(&(*dst)[0]);
    unsigned char* q = p;
    unsigned char c = 0;
    unsigned char t = 0;

    for (size_t i = 0; i < src.size(); i++)
    {
        if (src[i] == '=')
            break;
        // do
        // {
        //     if (src[i])
        //         c = Chr2Base(src[i]);
        //     else
        //         c = 65; // 字符串结束
        // } while (c == 64); // 跳过无效字符,如回车等

        if (src[i])
            c = Chr2Base(src[i]);
        else
            c = 65; // 字符串结束
        
        if (c == 64)
            return false;   // 无效字符，返回false

        if (c == 65)
            break;

        switch (i % 4)
        {
        case 0:
            t = static_cast<unsigned char>(c << 2);
            break;
        case 1:
            *p = t | static_cast<unsigned char>(c >> 4);
            p++;
            t = static_cast<unsigned char>(c << 4);
            break;
        case 2:
            *p = static_cast<unsigned char>(t | c >> 2);
            p++;
            t = static_cast<unsigned char>(c << 6);
            break;
        case 3:
            *p = t | c;
            p++;
            break;
        }
    }

    dst->resize(p - q);

    return true;
}

size_t Base64::Base64EncodeLen(size_t n)
{
    return (n + 2) / 3 * 4 + 1;
}

size_t Base64::Base64DecodeLen(size_t n)
{
    return n / 4 * 3 + 2;
}

} // end namespace common
