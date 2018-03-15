#ifndef __COMMON_URI_CODEC_H__
#define __COMMON_URI_CODEC_H__

#include <string>
#include <stdint.h>
#include <stdio.h>

namespace common
{

	#define CRC_TABLE_SIZE  64
	///_-
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

	inline std::string encode(uint64_t messId)
	{
		uint8_t index = 0;
		uint8_t bit_move = 6;
		uint8_t mod = 0x3f;
		std::string result = "";
		for(int i = 0; i < 11; i++)
		{
			//printf("messId = %ld\n",messId);
			index = static_cast<uint8_t>(messId & mod);
			//printf("i :: %d,index = %d\n",i,index);
			char sign = crc64tab[index];
			result.append(1,sign);
			messId = messId >> bit_move;
		}
		return result;
	}

	inline int crc_index(char c)
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

	//error return 0
	inline uint64_t decode(const std::string& messStr)
	{
		int index = 0;
		uint64_t messId = 0;
		if(messStr.length() != 11)
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