#ifndef __RCS_MYSQL_ENCRY_PASS_H__
#define __RCS_MYSQL_ENCRY_PASS_H__

#include <string>

namespace common
{

bool UpdateIniKeyValue(const char *section,const char *key,const char *value,const char *filename);
//AES ���ܺ�������ini�ļ������������Ľ���
//�����Ǵ�ini�ļ�������������
std::string aesDecrypt(std::string pass);

//AES ���ܺ������û�����������ܳ�����д��ini�ļ�
//�������û��������������
std::string aesEncryption(std::string pass);
}
#endif
