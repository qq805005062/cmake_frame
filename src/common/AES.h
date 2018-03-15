#ifndef __COMMON_AES_H__
#define __COMMON_AES_H__

#include <string>
#include <openssl/aes.h>
#include <iostream>

#include <common/noncopyable.h>

namespace common
{

class AES : common::noncopyable
{
public:
	AES() = default;
	~AES() = default;

	int encode(const std::string& key, const std::string& input, std::string& output);
	int decode(const std::string& key, const std::string& input, std::string& output);

	int cbcEncrypt(const std::string& key, const std::string& input, const std::string& initIV, std::string& output);
	int cbcDecrypt(const std::string& key, const std::string& input, const std::string& initIV, std::string& output);

private:
	AES_KEY key_;
};

} // end namespace common

#endif // __COMMON_AES_H__
