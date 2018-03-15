#ifndef __RCS_MYSQL_ENCRY_PASS_H__
#define __RCS_MYSQL_ENCRY_PASS_H__

#include <string>

namespace common
{

bool UpdateIniKeyValue(const char *section,const char *key,const char *value,const char *filename);
//AES 解密函数，从ini文件读出来的密文解密
//参数是从ini文件读出来的密码
std::string aesDecrypt(std::string pass);

//AES 加密函数，用户输入密码加密成密文写到ini文件
//参数是用户输入的明文密码
std::string aesEncryption(std::string pass);
}
#endif
