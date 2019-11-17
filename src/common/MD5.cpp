#include <openssl/md5.h>

#include "MD5.h"

#define PDEBUG(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

namespace common
{

void md5Encrpty(const std::string& input, unsigned char output[16])
{
    MD5(reinterpret_cast<const unsigned char*>(const_cast<char*>(input.data())), input.length(),output);
}

void stringToHexStr(const std::string& input, std::string& output)
{
    size_t inputLen = input.size(), outputLen = inputLen * 2;
    if(inputLen == 0)
    {
        output.assign("");
        return;
    }
    char *outbuf = new char[outputLen];
    if(outbuf)
    {
        unsigned char pInput = 0;
        char* oOut = outbuf;
        for (size_t i = 0; i < inputLen; ++i)
        {
            pInput = input.at(i);
            sprintf(oOut, "%02X", pInput);
            oOut++;oOut++;
        }
        output.assign(outbuf, outputLen);
        delete[] outbuf;
        return;
    }

    output.assign("");
    return;
}

void stringTohexStr(const std::string& input, std::string& output)
{
    size_t inputLen = input.size(), outputLen = inputLen * 2;
    if(inputLen == 0)
    {
        output.assign("");
        return;
    }
    char *outbuf = new char[outputLen];
    if(outbuf)
    {
        unsigned char pInput = 0;
        char* oOut = outbuf;
        for (size_t i = 0; i < inputLen; ++i)
        {
            pInput = input.at(i);
            sprintf(oOut, "%02x", pInput);
            oOut++;oOut++;
        }
        output.assign(outbuf, outputLen);
        delete[] outbuf;
        return;
    }

    output.assign("");
    return;
}

void stringMd5ToHexStr(const std::string& input, std::string& output)
{
    unsigned char mdbuf[16] = {0};
    md5Encrpty(input, mdbuf);
    std::string mdStr((char *)mdbuf, 16);
    stringToHexStr(mdStr, output);
}

void stringMd5TohexStr(const std::string& input, std::string& output)
{
    unsigned char mdbuf[16] = {0};
    md5Encrpty(input, mdbuf);
    std::string mdStr((char *)mdbuf, 16);
    stringTohexStr(mdStr, output);
}

}


