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
		:maxRspMicroSecond(0)
		,minRspMicreSecond(0)
		,allRspMicroSecond(0)
		,maxFullMicroSecond(0)
		,minFullMicroSecond(0)
		,allFullMicroSecond(0)
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
	
	void timeUsedCalculate(int64_t insertMicroSecond, int64_t beginMicroSecond, int64_t endMicroSecond)
	{
		int64_t con = endMicroSecond - beginMicroSecond, allconn = endMicroSecond - insertMicroSecond;
		printf("timeUsedCalculate insert begin end used all used %ld %ld %ld %ld %ld\n", insertMicroSecond, beginMicroSecond, endMicroSecond, con, allconn);
		SafeMutexLock lock(mutex_);
		allNum++;
		allRspMicroSecond += con;
		allFullMicroSecond += allconn;
		
		if(maxRspMicroSecond == 0 && minRspMicreSecond == 0)
		{
			maxRspMicroSecond = con;
			minRspMicreSecond = con;
		}

		if(maxFullMicroSecond == 0 && minFullMicroSecond == 0)
		{
			maxFullMicroSecond = allconn;
			minFullMicroSecond = allconn;
		}
		
		if(con > maxRspMicroSecond)
		{
			maxRspMicroSecond = con;
		}

		if(con < minRspMicreSecond)
		{
			minRspMicreSecond = con;
		}

		if(allconn > maxFullMicroSecond)
		{
			maxFullMicroSecond = allconn;
		}

		if(allconn < minFullMicroSecond)
		{
			minFullMicroSecond = allconn;
		}

		return;
	}

	void timeUsedStatistics()
	{
		char writeBuf[1024] = {0};
		
		int64_t microSecond_ = 0, allnum_ = 0, maxMicroSecond_ = 0, minMicroSecond_ = 0, averMicroSecond = 0, reqSpeed_ = 0, maxFullMicroSecond_ = 0, minFullMicroSecond_ = 0, allFullMicroSecond_ = 0, averFullMicroSecond_ = 0;
		{
			SafeMutexLock lock(mutex_);
			microSecond_ = allRspMicroSecond;
			allFullMicroSecond = allFullMicroSecond;
			allnum_ = allNum;
			maxMicroSecond_ = maxRspMicroSecond;
			minMicroSecond_ = minRspMicreSecond;

			maxFullMicroSecond_ = maxFullMicroSecond;
			minFullMicroSecond_ = minFullMicroSecond;
			allFullMicroSecond_ = allFullMicroSecond;
		}
		if(allnum_)
		{
			averMicroSecond = microSecond_ / allnum_;
			averFullMicroSecond_ = allFullMicroSecond_ / allnum_;
		}

		reqSpeed_ = reqSpeed.get();
		FILE* pFile = fopen("secondSummary","a+");
		sprintf(writeBuf, "timeUsedStatistics speed allNum maxRspMicroSecond minRspMicreSecond averRspMicroSecond %ld %ld %ld %ld %ld maxfullMicroSecond minfullMicreSecond averfullMicroSecond %ld %ld %ld\n", 
			(reqSpeed_ - reqSpeedL), allnum_, maxMicroSecond_, minMicroSecond_, averMicroSecond, maxFullMicroSecond_, minFullMicroSecond_, averFullMicroSecond_);
		size_t wLen = strlen(writeBuf);
		size_t nWri = fwrite( writeBuf, 1, wLen, pFile);
		fclose(pFile);
		reqSpeedL = reqSpeed_;
		printf("fwrite %ld writeBuf %s\n", nWri, writeBuf);
	}

private:
	volatile int64_t maxRspMicroSecond;
	volatile int64_t minRspMicreSecond;
	volatile int64_t allRspMicroSecond;

	volatile int64_t maxFullMicroSecond;
	volatile int64_t minFullMicroSecond;
	volatile int64_t allFullMicroSecond;
	
	volatile int64_t allNum;
	volatile int64_t reqSpeedL;
	AtomicInt64 reqSpeed;
	MutexLock mutex_;
	
};
}
#endif