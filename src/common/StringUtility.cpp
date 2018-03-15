#include <string.h>
#include <algorithm>

#include "common/StringUtility.h"

namespace common
{

void StringUtility::Split(const std::string& str,
                          const std::string& delim,
                          std::vector<std::string>* result)
{
	if (str.empty())
	{
		return;
	}

	if (delim[0] == '\0')
	{
		result->push_back(str);
		return;
	}

	size_t delimLength = delim.length();

	for (std::string::size_type beginIndex = 0; beginIndex < str.size();)
	{
		std::string::size_type endIndex = str.find(delim, beginIndex);
		if (endIndex == std::string::npos)
		{
			result->push_back(str.substr(beginIndex));
			return;
		}
		if (endIndex > beginIndex)
		{
			result->push_back(str.substr(beginIndex, (endIndex - beginIndex)));
		}

		beginIndex = endIndex + delimLength;
	}
}

bool StringUtility::StartsWith(const std::string& str, const std::string& prefix)
{
	if (prefix.length() > str.length())
	{
		return false;
	}
	if (memcmp(str.c_str(), prefix.c_str(), prefix.length()) == 0)
	{
		return true;
	}

	return false;
}

bool StringUtility::EndsWith(const std::string& str, const std::string& suffix)
{
	if (suffix.length() > str.length())
	{
		return false;
	}

	return (str.substr(str.length() - suffix.length()) == suffix);
}

std::string& StringUtility::Ltrim(std::string& str)
{
	std::string::iterator it =
	    find_if(str.begin(), str.end(), std::not1(std::ptr_fun(::isspace)));
	str.erase(str.begin(), it);
	return str;
}

std::string& StringUtility::Rtrim(std::string& str)
{
	std::string::reverse_iterator it =
	    find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun(::isspace)));
	str.erase(it.base(), str.end());
	return str;
}

std::string& StringUtility::Trim(std::string& str)
{
	return Rtrim(Ltrim(str));
}

void StringUtility::Trim(std::vector<std::string>* strList)
{
	if (nullptr == strList)
	{
		return;
	}

	std::vector<std::string>::iterator it;
	for (it = strList->begin(); it != strList->end(); ++it)
	{
		*it = Trim(*it);
	}
}

void StringUtility::StringReplace(const std::string& subStr1,
                                  const std::string& subStr2,
                                  std::string* str)
{
	std::string::size_type pos = 0;
	std::string::size_type a = subStr1.size();
	std::string::size_type b = subStr2.size();
	while ((pos = str->find(subStr1, pos)) != std::string::npos)
	{
		str->replace(pos, a, subStr2);
		pos += b;
	}
}

static const char ENCODECHARS[1024] = {
	3, '%', '0', '0', 3, '%', '0', '1', 3, '%', '0', '2', 3, '%', '0', '3',
	3, '%', '0', '4', 3, '%', '0', '5', 3, '%', '0', '6', 3, '%', '0', '7',
	3, '%', '0', '8', 3, '%', '0', '9', 3, '%', '0', 'A', 3, '%', '0', 'B',
	3, '%', '0', 'C', 3, '%', '0', 'D', 3, '%', '0', 'E', 3, '%', '0', 'F',
	3, '%', '1', '0', 3, '%', '1', '1', 3, '%', '1', '2', 3, '%', '1', '3',
	3, '%', '1', '4', 3, '%', '1', '5', 3, '%', '1', '6', 3, '%', '1', '7',
	3, '%', '1', '8', 3, '%', '1', '9', 3, '%', '1', 'A', 3, '%', '1', 'B',
	3, '%', '1', 'C', 3, '%', '1', 'D', 3, '%', '1', 'E', 3, '%', '1', 'F',
	1, '+', '2', '0', 3, '%', '2', '1', 3, '%', '2', '2', 3, '%', '2', '3',
	3, '%', '2', '4', 3, '%', '2', '5', 3, '%', '2', '6', 3, '%', '2', '7',
	3, '%', '2', '8', 3, '%', '2', '9', 3, '%', '2', 'A', 3, '%', '2', 'B',
	3, '%', '2', 'C', 1, '-', '2', 'D', 1, '.', '2', 'E', 3, '%', '2', 'F',
	1, '0', '3', '0', 1, '1', '3', '1', 1, '2', '3', '2', 1, '3', '3', '3',
	1, '4', '3', '4', 1, '5', '3', '5', 1, '6', '3', '6', 1, '7', '3', '7',
	1, '8', '3', '8', 1, '9', '3', '9', 3, '%', '3', 'A', 3, '%', '3', 'B',
	3, '%', '3', 'C', 3, '%', '3', 'D', 3, '%', '3', 'E', 3, '%', '3', 'F',
	3, '%', '4', '0', 1, 'A', '4', '1', 1, 'B', '4', '2', 1, 'C', '4', '3',
	1, 'D', '4', '4', 1, 'E', '4', '5', 1, 'F', '4', '6', 1, 'G', '4', '7',
	1, 'H', '4', '8', 1, 'I', '4', '9', 1, 'J', '4', 'A', 1, 'K', '4', 'B',
	1, 'L', '4', 'C', 1, 'M', '4', 'D', 1, 'N', '4', 'E', 1, 'O', '4', 'F',
	1, 'P', '5', '0', 1, 'Q', '5', '1', 1, 'R', '5', '2', 1, 'S', '5', '3',
	1, 'T', '5', '4', 1, 'U', '5', '5', 1, 'V', '5', '6', 1, 'W', '5', '7',
	1, 'X', '5', '8', 1, 'Y', '5', '9', 1, 'Z', '5', 'A', 3, '%', '5', 'B',
	3, '%', '5', 'C', 3, '%', '5', 'D', 3, '%', '5', 'E', 1, '_', '5', 'F',
	3, '%', '6', '0', 1, 'a', '6', '1', 1, 'b', '6', '2', 1, 'c', '6', '3',
	1, 'd', '6', '4', 1, 'e', '6', '5', 1, 'f', '6', '6', 1, 'g', '6', '7',
	1, 'h', '6', '8', 1, 'i', '6', '9', 1, 'j', '6', 'A', 1, 'k', '6', 'B',
	1, 'l', '6', 'C', 1, 'm', '6', 'D', 1, 'n', '6', 'E', 1, 'o', '6', 'F',
	1, 'p', '7', '0', 1, 'q', '7', '1', 1, 'r', '7', '2', 1, 's', '7', '3',
	1, 't', '7', '4', 1, 'u', '7', '5', 1, 'v', '7', '6', 1, 'w', '7', '7',
	1, 'x', '7', '8', 1, 'y', '7', '9', 1, 'z', '7', 'A', 3, '%', '7', 'B',
	3, '%', '7', 'C', 3, '%', '7', 'D', 1, '~', '7', 'E', 3, '%', '7', 'F',
	3, '%', '8', '0', 3, '%', '8', '1', 3, '%', '8', '2', 3, '%', '8', '3',
	3, '%', '8', '4', 3, '%', '8', '5', 3, '%', '8', '6', 3, '%', '8', '7',
	3, '%', '8', '8', 3, '%', '8', '9', 3, '%', '8', 'A', 3, '%', '8', 'B',
	3, '%', '8', 'C', 3, '%', '8', 'D', 3, '%', '8', 'E', 3, '%', '8', 'F',
	3, '%', '9', '0', 3, '%', '9', '1', 3, '%', '9', '2', 3, '%', '9', '3',
	3, '%', '9', '4', 3, '%', '9', '5', 3, '%', '9', '6', 3, '%', '9', '7',
	3, '%', '9', '8', 3, '%', '9', '9', 3, '%', '9', 'A', 3, '%', '9', 'B',
	3, '%', '9', 'C', 3, '%', '9', 'D', 3, '%', '9', 'E', 3, '%', '9', 'F',
	3, '%', 'A', '0', 3, '%', 'A', '1', 3, '%', 'A', '2', 3, '%', 'A', '3',
	3, '%', 'A', '4', 3, '%', 'A', '5', 3, '%', 'A', '6', 3, '%', 'A', '7',
	3, '%', 'A', '8', 3, '%', 'A', '9', 3, '%', 'A', 'A', 3, '%', 'A', 'B',
	3, '%', 'A', 'C', 3, '%', 'A', 'D', 3, '%', 'A', 'E', 3, '%', 'A', 'F',
	3, '%', 'B', '0', 3, '%', 'B', '1', 3, '%', 'B', '2', 3, '%', 'B', '3',
	3, '%', 'B', '4', 3, '%', 'B', '5', 3, '%', 'B', '6', 3, '%', 'B', '7',
	3, '%', 'B', '8', 3, '%', 'B', '9', 3, '%', 'B', 'A', 3, '%', 'B', 'B',
	3, '%', 'B', 'C', 3, '%', 'B', 'D', 3, '%', 'B', 'E', 3, '%', 'B', 'F',
	3, '%', 'C', '0', 3, '%', 'C', '1', 3, '%', 'C', '2', 3, '%', 'C', '3',
	3, '%', 'C', '4', 3, '%', 'C', '5', 3, '%', 'C', '6', 3, '%', 'C', '7',
	3, '%', 'C', '8', 3, '%', 'C', '9', 3, '%', 'C', 'A', 3, '%', 'C', 'B',
	3, '%', 'C', 'C', 3, '%', 'C', 'D', 3, '%', 'C', 'E', 3, '%', 'C', 'F',
	3, '%', 'D', '0', 3, '%', 'D', '1', 3, '%', 'D', '2', 3, '%', 'D', '3',
	3, '%', 'D', '4', 3, '%', 'D', '5', 3, '%', 'D', '6', 3, '%', 'D', '7',
	3, '%', 'D', '8', 3, '%', 'D', '9', 3, '%', 'D', 'A', 3, '%', 'D', 'B',
	3, '%', 'D', 'C', 3, '%', 'D', 'D', 3, '%', 'D', 'E', 3, '%', 'D', 'F',
	3, '%', 'E', '0', 3, '%', 'E', '1', 3, '%', 'E', '2', 3, '%', 'E', '3',
	3, '%', 'E', '4', 3, '%', 'E', '5', 3, '%', 'E', '6', 3, '%', 'E', '7',
	3, '%', 'E', '8', 3, '%', 'E', '9', 3, '%', 'E', 'A', 3, '%', 'E', 'B',
	3, '%', 'E', 'C', 3, '%', 'E', 'D', 3, '%', 'E', 'E', 3, '%', 'E', 'F',
	3, '%', 'F', '0', 3, '%', 'F', '1', 3, '%', 'F', '2', 3, '%', 'F', '3',
	3, '%', 'F', '4', 3, '%', 'F', '5', 3, '%', 'F', '6', 3, '%', 'F', '7',
	3, '%', 'F', '8', 3, '%', 'F', '9', 3, '%', 'F', 'A', 3, '%', 'F', 'B',
	3, '%', 'F', 'C', 3, '%', 'F', 'D', 3, '%', 'F', 'E', 3, '%', 'F', 'F',
};

void StringUtility::UrlEncode(const std::string& srcStr, std::string* dstStr)
{
	dstStr->clear();
	for (size_t i = 0; i < srcStr.length(); i++)
	{
		unsigned short offset = static_cast<unsigned short>(srcStr[i] * 4);
		dstStr->append((ENCODECHARS + offset + 1), ENCODECHARS[offset]);
	}
}

static const char HEX2DEC[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

void StringUtility::UrlDecode(const std::string& srcStr, std::string* dstStr)
{
	dstStr->clear();
	const unsigned char* srcBegin = reinterpret_cast<const unsigned char*>(srcStr.data());
	const unsigned char* srcEnd = srcBegin + srcStr.length();
	const unsigned char* srcLast = srcEnd - 2;

	while (srcBegin < srcLast)
	{
		if ((*srcBegin) == '%')
		{
			char dec1, dec2;
			if (-1 != (dec1 = HEX2DEC[*(srcBegin + 1)])
			        && -1 != (dec2 = HEX2DEC[*(srcBegin + 2)]))
			{
				dstStr->append(1, (dec1 << 4) + dec2);
				srcBegin += 3;
				continue;
			}
		}
		else if ((*srcBegin) == '+')
		{
			dstStr->append(1, ' ');
			++srcBegin;
			continue;
		}
		dstStr->append(1, static_cast<char>(*srcBegin));
		++srcBegin;
	}

	while (srcBegin < srcEnd)
	{
		dstStr->append(1, static_cast<char>(*srcBegin));
		++srcBegin;
	}
}

void StringUtility::ToUpper(std::string* str)
{
	std::transform(str->begin(), str->end(), str->begin(), ::toupper);
}

void StringUtility::ToLower(std::string* str)
{
	std::transform(str->begin(), str->end(), str->begin(), ::tolower);
}

bool StringUtility::StripSuffix(std::string* str, const std::string& suffix)
{
	if (str->length() >= suffix.length())
	{
		size_t suffixPos = str->length() - suffix.length();
		if (str->compare(suffixPos, std::string::npos, suffix) == 0)
		{
			str->resize(str->size() - suffix.size());
			return true;
		}
	}

	return false;
}

bool StringUtility::StripPrefix(std::string* str, const std::string& prefix)
{
	if (str->length() >= prefix.length())
	{
		if (str->substr(0, prefix.size()) == prefix)
		{
			*str = str->substr(prefix.size());
			return true;
		}
	}

	return false;
}

#if defined(__clang__) || __GNUC_PREREQ (4,6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion"

bool StringUtility::Hex2Bin(const char* hexStr, std::string* binStr)
{
	if (nullptr == hexStr || nullptr == binStr)
	{
		return false;
	}

	binStr->clear();
	while (*hexStr != '\0')
	{
		if (hexStr[1] == '\0')
		{
			return false;
		}

		uint8_t high = static_cast<uint8_t>(hexStr[0]);
		uint8_t low  = static_cast<uint8_t>(hexStr[1]);
#define ASCII2DEC(c) \
		if (c >= '0' && c<= '9') c -= '0'; \
		else if (c >= 'A' && c <= 'F') c -= ('A' - 10); \
		else if (c >= 'a' && c <= 'f') c -= ('a' - 10); \
		else return false

		ASCII2DEC(high);
		ASCII2DEC(low);
		binStr->append(1, static_cast<char>((high << 4) + low));
		hexStr += 2;
	}

	return true;
}

bool StringUtility::Bin2Hex(const char* binStr, size_t binLen, std::string* hexStr)
{
	if (nullptr == binStr || nullptr == hexStr)
	{
		return false;
	}
	hexStr->clear();

	for (size_t i = 0; i < binLen; i++)
	{
		uint8_t high = (static_cast<uint8_t>(binStr[i]) >> 4);
		uint8_t low  = (static_cast<uint8_t>(binStr[i]) & 0xF);
#define DEC2ASCII(c) \
		if (c <= 9) c += '0'; \
		else c += ('A' - 10)

		DEC2ASCII(high);
		DEC2ASCII(low);
		hexStr->append(1, static_cast<char>(high));
		hexStr->append(1, static_cast<char>(low));
		// binStr += 1;
	}
	return true;
}

#if defined(__clang__) || __GNUC_PREREQ (4,6)
#pragma GCC diagnostic pop
#else
#pragma GCC diagnostic warning "-Wconversion"
#endif

} // end namespace common
