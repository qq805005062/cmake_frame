#ifndef __REMAIN_MGR_BASE64_H__
#define __REMAIN_MGR_BASE64_H__

#include <string>

namespace REMAIN_MGR
{

class Base64
{
public:
	Base64() = default;
	~Base64() = default;

	static bool Encode(const std::string& src, std::string* dst);
	static bool Decode(const std::string& src, std::string* dst);

private:
	// 根据Base64编码表中的序号求的某个字符
	static inline char Base2Chr(unsigned char n);
	// 求的某个字符在Base64码表中的序号
	static inline unsigned char Chr2Base(char c);

	inline static size_t Base64EncodeLen(size_t n);
	inline static size_t Base64DecodeLen(size_t n);

};

} // end namespace REMAIN_MGR

#endif // __REMAIN_MGR_BASE64_H__

