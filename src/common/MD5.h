#ifndef __COMMON_MD_FIVE_H__
#define __COMMON_MD_FIVE_H__

#include <string>

//#include "MD5.h"

namespace common
{

/*
 * [md5Encrpty] md5压缩数据，最原始的，产生16个字节数据
 * @author xiaoxiao 2019-08-23
 * @param input 需要压缩的数据
 * @param output 出参，压缩之后生成的16个字节的数据
 * @param
 *
 * @return 无，不会失败的
 */
void md5Encrpty(const std::string& input, unsigned char output[16]);

/*
 * [stringToHexStr] 二进制流数据格式化成大写的16进制数据
 * @author xiaoxiao 2019-08-23
 * @param input 入参，需要格式化16进制的字符串
 * @param output 出参，格式化为16进制之后的字符串
 * @param
 *
 * @return 无，如果出错为空，可能内部malloc错了，注意打印入参长度
 */
void stringToHexStr(const std::string& input, std::string& output);

/*
 * [stringTohexStr] 二进制流数据格式化成小写的16进制数据
 * @author xiaoxiao 2019-08-23
 * @param input 入参，需要格式化16进制的字符串
 * @param output 出参，格式化为16进制之后的字符串
 * @param
 *
 * @return 无，如果出错为空，可能内部malloc错了，注意打印入参长度
 */
void stringTohexStr(const std::string& input, std::string& output);

/*
 * [stringMd5ToHexStr] md5压缩数据，之后格式化大写的16进制数据
 * @author xiaoxiao 2019-08-23
 * @param input 需要md5之后格式化16进制的数据
 * @param output md5之后格式化16进制大写的结果
 * @param
 *
 * @return 无，md5不会出错，如果出参为空，就是内部malloc出错了
 */
void stringMd5ToHexStr(const std::string& input, std::string& output);

/*
 * [stringMd5ToHexStr] md5压缩数据，之后格式化小写的16进制数据
 * @author xiaoxiao 2019-08-23
 * @param input 需要md5之后格式化16进制的数据
 * @param output md5之后格式化16进制小写的结果
 * @param
 *
 * @return 无，md5不会出错，如果出参为空，就是内部malloc出错了
 */
void stringMd5TohexStr(const std::string& input, std::string& output);

}

#endif
