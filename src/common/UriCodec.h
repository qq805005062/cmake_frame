#ifndef __COMMON_URI_CODEC_H__
#define __COMMON_URI_CODEC_H__

#include <string>
#include <stdint.h>
#include <stdio.h>

#include <string.h>
#include <cstring>
namespace common
{

	#define CRC_TABLE_SIZE  64
	#define CRC_URISTR_LEN	11

	#define ZEOR_ASCII_CODE 48
	#define NINE_ASCII_CODE	57

	#define ADD_SIGN_ASCII_CODE	64 // @
	///_-
	////+ 43 0x2b
	static const char crc64tab[CRC_TABLE_SIZE] = {
		0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
		0x38,0x39,0x41,0x42,0x43,0x44,0x45,0x46,
		0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,
		0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,
		0x57,0x58,0x59,0x5a,0x61,0x62,0x63,0x64,
		0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,
		0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,
		0x75,0x76,0x77,0x78,0x79,0x7a,0x5f,0x2d
	};
	//根据msgid加密生成uri
	inline std::string encode(uint64_t messId)
	{
		uint8_t index = 0;
		uint8_t bit_move = 6;
		uint8_t mod = 0x3f;
		char resBuf[16] = {0},*pChar = resBuf;
		for(int i = 0; i < CRC_URISTR_LEN; i++)
		{
			//printf("messId = %ld\n",messId);
			index = static_cast<uint8_t>(messId & mod);
			//printf("i :: %d,index = %d\n",i,index);
			*pChar = crc64tab[index];
			pChar++;
			messId = messId >> bit_move;
		}
		std::string result(resBuf);
		return result;
	}

	inline int crc_index(const char c)
	{
		int sub = 0;
		for(;sub < CRC_TABLE_SIZE;sub++)
		{
			if(c == crc64tab[sub])
				break;
		}
		
		if(sub >= CRC_TABLE_SIZE)
			return -1;
		return sub;
	}

	//根据uri反解除msgid
	//error return 0
	inline uint64_t decode(const std::string& messStr)
	{
		int index = 0;
		uint64_t messId = 0;
		if(messStr.length() != CRC_URISTR_LEN)
			return 0;
		for(int i = 10;i >= 0;i--)
		{
			index = crc_index(messStr.at(i));
			//printf("messStr.at(i) :: %c ,i :: %d,index = %d\n",messStr.at(i),i,index);
			if(index < 0)
				return 0;
			messId = messId | crc_index(messStr[i]);
			//printf("messId = %lu\n",messId);
			if(i > 0)
				messId  = messId << 6;
			//printf("messId = %lu\n",messId);
		}
		return messId;
	}

	//将uri和电话号码加密成一个字符串,手机最长不能超过21位。
	inline std::string uriPhoneEncode(const std::string& u,const std::string& phoneNum)
	{
		size_t sub = 0;
		int index = 0;
		char buff[128] = {0},*pChar = buff,*pp = buff;
		const char *qChar = phoneNum.c_str();
		size_t numLen = phoneNum.size();
		memcpy(pChar,u.c_str(),CRC_URISTR_LEN);
		pChar += CRC_URISTR_LEN;
		for(size_t i = 0; i < numLen; i++)
		{
			if(*qChar < ZEOR_ASCII_CODE || *qChar > NINE_ASCII_CODE)
			{
				*pChar = ADD_SIGN_ASCII_CODE;
				pChar++;
				//sub = *qChar;
				sub = *qChar / CRC_TABLE_SIZE;
				*pChar = crc64tab[sub];
				pChar++;
				//index = *qChar;
				index = *qChar % CRC_TABLE_SIZE;
				*pChar = crc64tab[index];
			}else{
				sub = i % CRC_URISTR_LEN;
				index = crc_index(pp[sub]);
				index += (*qChar - ZEOR_ASCII_CODE);
				index = index % CRC_TABLE_SIZE;
				*pChar = crc64tab[index];
			}
			pChar++;
			qChar++;
		}
		std::string uriPhone(buff,strlen(buff));
		return uriPhone;
	}

	//由电话号码与uri解密成的字符串解除uri，和电话号码，加密字符串最长不能超过74
	inline int uriPhoneDecode(const std::string& encondeStr,std::string& u, std::string& phoneNum)
	{
		int sub = 0,index = 0;
		char buff[128] = {0},*pp = buff;
		size_t numLen = encondeStr.size();
		if(numLen < CRC_URISTR_LEN)
			return -1;
		numLen -= CRC_URISTR_LEN;
		const char *pChar = encondeStr.c_str(),*qChar = pChar;
		u.assign(pChar,CRC_URISTR_LEN);
		
		pChar += CRC_URISTR_LEN;
		for(size_t i = 0;i < numLen; i++)
		{
			if(i % CRC_URISTR_LEN == 0)
				qChar = encondeStr.c_str();
			
			if(*pChar == ADD_SIGN_ASCII_CODE)
			{
				numLen -= 2;
				pChar++;
				index = crc_index(*pChar);
				pChar++;
				sub = crc_index(*pChar);
				if(index < 0 || sub < 0)
					return index;
				*pp = static_cast<char>(index * CRC_TABLE_SIZE + sub);
			}else{
				
index = crc_index(*pChar);
				sub = crc_index(*qChar);
				if(index < 0 || sub < 0)
					return index;
				if(index >= sub)
					*pp = static_cast<char>(index -sub + ZEOR_ASCII_CODE);
				else
					*pp = static_cast<char>(index - sub + CRC_TABLE_SIZE + ZEOR_ASCII_CODE);
			}
			pChar++;
			pp++;
			qChar++;				
		}

		phoneNum.assign(buff,strlen(buff));
		return 0;
	}
/*
int main(int argc,char *argv[])
{
	//uint64_t num = 9223372036854775807;
	//uint64_t num = 0x6ff5ff4ffffff3ff;
	uint64_t num = 0xffffffffffffffff;
	printf("num = %ld\n",num);

	std::string uri = encode(num);
	printf("uri = %s\n",uri.c_str());

	uint64_t messid = decode(uri);
	printf("messid = %lu\n",messid);

	return 0;
}
*/

}

#endif