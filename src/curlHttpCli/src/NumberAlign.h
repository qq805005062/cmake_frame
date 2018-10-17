#ifndef __XIAO_NUMBER_ALIGN_H__
#define __XIAO_NUMBER_ALIGN_H__
#include <string.h>

#include "Atomic.h"
#include "noncopyable.h"
#include "Singleton.h"
#include "MutexLock.h"

namespace CURL_HTTP_CLI
{
class AsyncQueueNum : public noncopyable
{
public:
	AsyncQueueNum()
		:asyncNum()
	{
	}
	
	~AsyncQueueNum()
	{
	}

	static AsyncQueueNum& instance() { return Singleton<AsyncQueueNum>::instance();}

	void asyncReq()
	{
		asyncNum.increment();
	}

	void asyncRsp()
	{
		asyncNum.decrement();
	}

	int64_t asyncQueNum()
	{
		return asyncNum.get();
	}
private:
	
	AtomicInt64 asyncNum;
};

class TimeUsedUp : public noncopyable
{
public:
	TimeUsedUp()
		:maxMicroSecond(0)
		,minMicreSecond(0)
		,allMincroSecond(0)
		,allNum(0)
		,reqSpeedL(0)
		,reqSpeed()
		,mutex_()
	{
	}

	~TimeUsedUp()
	{
	}

	static TimeUsedUp& instance() { return Singleton<TimeUsedUp>::instance();}

	void requestNumAdd()
	{
		reqSpeed.increment();
	}
	
	void timeUsedCalculate(int64_t beginMincroSecond, int64_t endMincroSecond)
	{
		int64_t con = endMincroSecond - beginMincroSecond;
		printf("timeUsedCalculate con %ld %ld %ld\n", beginMincroSecond, endMincroSecond, con);
		MutexLockGuard lock(mutex_);
		allNum++;
		allMincroSecond += con;

		if(maxMicroSecond == 0 && minMicreSecond == 0)
		{
			minMicreSecond = con;
			maxMicroSecond = con;
			return;
		}
		
		if(con > maxMicroSecond)
		{
			maxMicroSecond = con;
			return;
		}

		if(con < minMicreSecond)
		{
			minMicreSecond = con;
			return;
		}
		
		return;
	}

	void timeUsedStatistics()
	{
		char writeBuf[1024] = {0};
		
		int64_t microSecond_ = 0, allnum_ = 0, maxMicroSecond_ = 0, minMicroSecond_ = 0, averMicroSecond = 0, reqSpeed_ = 0;
		{
			MutexLockGuard lock(mutex_);
			microSecond_ = allMincroSecond;
			allnum_ = allNum;
			maxMicroSecond_ = maxMicroSecond;
			minMicroSecond_ = minMicreSecond;
		}
		if(allnum_)
		{
			averMicroSecond = microSecond_ / allnum_;
		}

		reqSpeed_ = reqSpeed.get();
		FILE* pFile = fopen("secondSummary","a+");
		sprintf(writeBuf, "timeUsedStatistics speed allNum maxMicroSecond minMicreSecond averMicroSecond %ld %ld %ld %ld %ld \n", (reqSpeed_ - reqSpeedL), allnum_, maxMicroSecond_, minMicroSecond_, averMicroSecond );
		size_t wLen = strlen(writeBuf);
		size_t nWri = fwrite( writeBuf, 1, wLen, pFile);
		fclose(pFile);
		reqSpeedL = reqSpeed_;
		printf("fwrite %ld writeBuf %s\n", nWri, writeBuf);
	}

private:
	volatile int64_t maxMicroSecond;
	volatile int64_t minMicreSecond;
	volatile int64_t allMincroSecond;
	volatile int64_t allNum;
	volatile int64_t reqSpeedL;
	AtomicInt64 reqSpeed;
	MutexLock mutex_;
	
};
}
#endif