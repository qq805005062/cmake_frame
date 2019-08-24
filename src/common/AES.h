#ifndef __COMMON_AES_H__
#define __COMMON_AES_H__

#include <string>

//#include "AES.h"
namespace common
{

/*
 * [aesCbcEncrypt] AES cbc加密，注意加密之后的字符串是二进制流，要想打印要么base64，要么格式化16进制
 * @author xiaoxiao 2019-08-23
 * @param input 需要AES cbc加密的字符串
 * @param key 加密用的key，必须是16个字节长度，最好是可打印字符串
 * @param initIV 加密用的偏移量，必须是16个字节长度，最好是可打印字符串
 * @param output 出参，加密之后的密文，二进制流
 * @param alignType 对齐方式 0是补0，1是补充缺失长度,如果压缩二进制流数据必须选择1
 * @param keyBits key的比特位，默认是128，也可以是256
 *
 * @return 0是成功，其他是失败
 */
int aesCbcEncrypt(const std::string& input, const std::string& key, const std::string& initIV, std::string& output, int alignType = 1, int keyBits = 128);

/*
 * [aesCbcDecrypt] AES cbc解密
 * @author xiaoxiao 2019-08-23
 * @param input 需要AES cbc解密的字符串，二进制流
 * @param key 解密用的key，必须是和加密用的key一模一样
 * @param initIV 解密用的偏移量，必须和加密使用的iv一模一样
 * @param output 出参，解密之后的字符串
 * @param alignType 对齐方式 0是补0，1是补充缺失长度，必须与加密保持一致
 * @param keyBits key的比特位，默认是128，也可以是256，必须要与解密一致，尤其联调的时候要注意
 *
 * @return 0是成功，其他是失败
 */
int aesCbcDecrypt(const std::string& input, const std::string& key, const std::string& initIV, std::string& output, int alignType = 1, int keyBits = 128);



/*
 * [aesEcbEncrypt] AES ecb加密，注意加密之后的字符串是二进制流，要想打印要么base64，要么格式化16进制
 * @author xiaoxiao 2019-08-23
 * @param input 需要AES ecb加密的字符串
 * @param key 加密用的key，必须是16个字节长度，最好是可打印字符串
 * @param output 出参，加密之后的密文，二进制流
 * @param alignType 对齐方式 0是补0，1是补充缺失长度,如果压缩二进制流数据必须选择1
 * @param keyBits key的比特位，默认是128，也可以是256
 *
 * @return 0是成功，其他是失败
 */
int aesEcbEncrypt(const std::string& input, const std::string& key, std::string& output, int alignType = 1, int keyBits = 128);

/*
 * [aesEcbDecrypt] AES ecb解密
 * @author xiaoxiao 2019-08-23
 * @param input 需要AES ecb加密的字符串
 * @param key 解密用的key，必须是和加密用的key一模一样
 * @param output 出参，解密之后的字符串
 * @param alignType 对齐方式 0是补0，1是补充缺失长度，必须与加密保持一致
 * @param keyBits key的比特位，默认是128，也可以是256，必须要与解密一致，尤其联调的时候要注意
 *
 * @return 0是成功，其他是失败
 */
int aesEcbDecrypt(const std::string& input, const std::string& key, std::string& output, int alignType = 1, int keyBits = 128);

}
#endif
