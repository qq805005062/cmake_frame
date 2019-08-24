#include <string.h>

#include <openssl/aes.h>

#include "AES.h"

#define PDEBUG(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

//#define AES_BLOCK_SIZE                (16)

namespace common
{

inline void aes_cbcEncrypt(char input[], int len, const unsigned char key[16], unsigned char iv[16], char output[], int keyBits)
{
    AES_KEY aes_key;
    AES_set_encrypt_key(key, keyBits, &aes_key);//第二个参数可以是128或者256，加解密要保持一致，不然是没有办法解密的
    AES_cbc_encrypt((const unsigned char* )input, (unsigned char* )output, (unsigned long)len, &aes_key, iv, AES_ENCRYPT); 
}

//解密
inline void aes_cbcDecrypt(char input[], int len, const unsigned char key[16], unsigned char iv[16], char output[ ], int keyBits)
{
    AES_KEY aes_key;
    AES_set_encrypt_key(key, keyBits, &aes_key);//第二个参数可以是128或者256，加解密要保持一致，不然是没有办法解密的
    AES_cbc_encrypt((const unsigned char *)input, (unsigned char* )output, (unsigned long)len, &aes_key, iv, AES_DECRYPT);
}

inline void aes_ecbEncrypt(const std::string& input, const unsigned char key[16], std::string& ouput, int keyBits)
{
    AES_KEY aes_key;
    AES_set_encrypt_key(key, keyBits, &aes_key);//第二个参数可以是128或者256，加解密要保持一致，不然是没有办法解密的

    size_t inputBlockSize = input.length() / AES_BLOCK_SIZE;
    for (size_t i = 0; i < inputBlockSize; i++)
    {
        unsigned char outbuf[AES_BLOCK_SIZE] = { 0 };
        AES_encrypt(reinterpret_cast<const unsigned char* >(input.c_str() + i * AES_BLOCK_SIZE), outbuf, &aes_key);
        ouput.append(reinterpret_cast<char* >(outbuf), AES_BLOCK_SIZE);
    }
}

inline void aes_ecbDecrypt(const std::string& input, const unsigned char key[16], std::string& ouput, int keyBits)
{
    AES_KEY aes_key;
    AES_set_encrypt_key(key, keyBits, &aes_key);//第二个参数可以是128或者256，加解密要保持一致，不然是没有办法解密的

    size_t inputBlockSize = input.length() / AES_BLOCK_SIZE;
    for (size_t i = 0; i < inputBlockSize; i++)
    {
        unsigned char outbuf[AES_BLOCK_SIZE] = { 0 };
        AES_decrypt(reinterpret_cast<const unsigned char* >(input.c_str() + i * AES_BLOCK_SIZE), outbuf, &aes_key);
        ouput.append(reinterpret_cast<char* >(outbuf), AES_BLOCK_SIZE);
    }
}


int aesCbcEncrypt(const std::string& input, const std::string& key, const std::string& initIV, std::string& output, int alignType, int keyBits)
{
    unsigned char aes_key[AES_BLOCK_SIZE] = {0}, aes_iv[AES_BLOCK_SIZE] = {0};
    int inLen = 0, outLen = 0;
    inLen = input.length();
    output.assign("");
    if((inLen == 0) || (key.length() != AES_BLOCK_SIZE) || (initIV.length() != AES_BLOCK_SIZE))
    {
        PERROR("The input will be encrypt:key:initiv len %lu:%lu:%lu is error ,can't be encrypt", input.length(), key.length(), initIV.length());
        return -1;
    }
    outLen = (inLen / AES_BLOCK_SIZE) * AES_BLOCK_SIZE + AES_BLOCK_SIZE;

    memcpy(aes_key, key.c_str(),AES_BLOCK_SIZE);
    memcpy(aes_iv, initIV.c_str(),AES_BLOCK_SIZE);
    //明文、密文
    char *p_expre = new char[outLen], *q_encry = new char[outLen];
    if(p_expre && q_encry)
    {
        memset(p_expre,0,outLen);
        memset(q_encry,0,outLen);
        memcpy(p_expre, input.c_str(),inLen);
        if(alignType == 1)
        {
            int padding = outLen - inLen;
            memset((unsigned char*)(p_expre + inLen), padding, padding);
        }
        
        aes_cbcEncrypt( p_expre, outLen, aes_key, aes_iv, q_encry, keyBits);
        output.assign(q_encry,outLen);
        delete[] p_expre;
        delete[] q_encry;
    }else{
        if(p_expre)
        {
            delete[] p_expre;
        }
        if(q_encry)
        {
            delete[] q_encry;
        }        return -2;
    }

    return 0;
}

int aesCbcDecrypt(const std::string& input, const std::string& key, const std::string& initIV, std::string& output, int alignType, int keyBits)
{
    unsigned char aes_key[AES_BLOCK_SIZE] = {0}, aes_iv[AES_BLOCK_SIZE] = {0};
    int len = input.length(), out_len = 0;
    output.assign("");

    if((input.empty()) || (input.length() % AES_BLOCK_SIZE != 0))
    {
        PERROR("The input will be decrypt len %lu is error ,can't be decode", input.length());
        return -1;
    }
    if((key.length() != AES_BLOCK_SIZE) || (initIV.length() != AES_BLOCK_SIZE))
    {
        PERROR("The input will be decrypt key:initIv %lu:%lu is error ,can't be decrypt", key.length(), initIV.length());
        return -1;
    }

    memcpy(aes_key, key.c_str(), AES_BLOCK_SIZE);
    memcpy(aes_iv, initIV.c_str(), AES_BLOCK_SIZE);

    //明文、密文
    char *p_expre = new char[len], *q_encry = new char[len];
    if(p_expre && q_encry)
    {
        memset(p_expre,0,len);
        memset(q_encry,0,len);
        memcpy(q_encry,input.c_str(),len);
        aes_cbcDecrypt(q_encry, len, aes_key, aes_iv, p_expre, keyBits);
        
        out_len = strlen(p_expre);
        if(alignType == 1)
        {
            int padding = p_expre[len - 1];
            if((out_len != len) ||(padding < 0) || (padding > AES_BLOCK_SIZE))
            {
                delete[] p_expre;
                delete[] q_encry;
                return -3;
            }
            out_len = out_len - padding;
        }
        
        output.append(p_expre,out_len);
        delete[] p_expre;
        delete[] q_encry;
    }else{
        if(p_expre)
        {
            delete[] p_expre;
        }

        if(q_encry)
        {
            delete[] q_encry;
        }
        return -2;
    }
    
    return 0;
}

int aesEcbEncrypt(const std::string& input, const std::string& key, std::string& output, int alignType, int keyBits)
{
    unsigned char aes_key[AES_BLOCK_SIZE] = {0};
    int inLen = 0, outLen = 0;
    inLen = input.length();
    output.assign("");
    if((inLen == 0) || (key.length() != AES_BLOCK_SIZE))
    {
        PERROR("The input will be encrypt:key len %lu:%lu is error ,can't be encrypt", input.length(), key.length());
        return -1;
    }
    outLen = (inLen / AES_BLOCK_SIZE) * AES_BLOCK_SIZE + AES_BLOCK_SIZE;
    memcpy(aes_key, key.c_str(),AES_BLOCK_SIZE);
    //明文、密文
    char *p_expre = new char[outLen];
    if(p_expre)
    {
        memset(p_expre,0,outLen);
        memcpy(p_expre, input.c_str(),inLen);
        if(alignType == 1)
        {
            int padding = outLen - inLen;
            memset((unsigned char*)(p_expre + inLen), padding, padding);
        }

        std::string tmpExpreStr(p_expre, outLen);
        aes_ecbEncrypt( tmpExpreStr, aes_key, output, keyBits);
        delete[] p_expre;
    }else{
        return -2;
    }

    return 0;
}

int aesEcbDecrypt(const std::string& input, const std::string& key, std::string& output, int alignType, int keyBits)
{
    unsigned char aes_key[AES_BLOCK_SIZE] = {0};
    int len = input.length(),out_len = 0;
    output.assign("");

    if((input.empty()) || (input.length() % AES_BLOCK_SIZE != 0))
    {
        PERROR("The input will be decrypt len %lu is error ,can't be decode", input.length());
        return -1;
    }
    if((key.length() != AES_BLOCK_SIZE))
    {
        PERROR("The input will be decrypt key %lu is error ,can't be decrypt", key.length());
        return -1;
    }

    memcpy(aes_key, key.c_str(), AES_BLOCK_SIZE);

    std::string tmpOutPut = "";
    aes_ecbDecrypt(input, aes_key, tmpOutPut, keyBits);

    int outLen = strlen(tmpOutPut.c_str());
    if(alignType == 1)
    {
        int padding = tmpOutPut.at(tmpOutPut.length() - 1);
        if((out_len != len) ||(padding < 0) || (padding > AES_BLOCK_SIZE))
        {
            return -3;
        }
        outLen = outLen - padding;
    }

    output.assign(tmpOutPut.c_str(), outLen);

    return 0;
}


}

