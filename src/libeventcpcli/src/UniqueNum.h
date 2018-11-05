#ifndef __LIBEVENT_TCPCLI_UNIQUE_NUM_H__
#define __LIBEVENT_TCPCLI_UNIQUE_NUM_H__
//#include "UniqueNum.h"
#include <stdint.h>
#include "Atomic.h"

static LIBEVENT_TCP_CLI::AtomicUInt32 UniqueTail;

//0 ~ 8191
inline uint64_t uniqueNumId(uint32_t uniquId = 60)
{
	unsigned int nYear = 0;
	unsigned int nMonth = 0;
	unsigned int nDay = 0;
	unsigned int nHour = 0;
	unsigned int nMin = 0;
	unsigned int nSec = 0;
	unsigned int nNodeid = uniquId % 8191;
	unsigned int nNo = static_cast<unsigned int>(UniqueTail.incrementAndGet() % (0x03ffff));

  	struct timeval tv;
	struct tm	   tm_time;
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm_time);

	nYear = tm_time.tm_year+1900;
	nYear = nYear%100;
	nMonth = tm_time.tm_mon+1;
	nDay = tm_time.tm_mday;
	nHour = tm_time.tm_hour;
	nMin = tm_time.tm_min;
	nSec = tm_time.tm_sec;

	uint64_t j = 0;
	j |= static_cast<int64_t>(nYear& 0x7f) << 57;   //year 0~99
	j |= static_cast<int64_t>(nMonth & 0x0f) << 53;//month 1~12
	j |= static_cast<int64_t>(nDay & 0x1f) << 48;//day 1~31
	j |= static_cast<int64_t>(nHour & 0x1f) << 43;//hour 0~24
	j |= static_cast<int64_t>(nMin & 0x3f) << 37;//min 0~59
	j |= static_cast<int64_t>(nSec & 0x3f) << 31;//second 0~59
	j |= static_cast<int64_t>(nNodeid & 0x01fff) << 18;//nodeid 1~8000
	j |= static_cast<int64_t>(nNo & 0x03ffff);	//seqid,0~0x03ffff

	return j;
}

#endif
