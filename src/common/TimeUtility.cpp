#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "common/TimeUtility.h"

namespace common
{

int64_t TimeUtility::GetCurrentMS()
{
	int64_t timestamp = GetCurrentUS();
	return timestamp / 1000;
}

int64_t TimeUtility::GetCurrentUS()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	int64_t timestamp = tv.tv_sec * 1000000 + tv.tv_usec;
	return timestamp;
}

std::string TimeUtility::GetStringTime()
{
	time_t now = time(NULL);
	struct tm tmNow;
	struct tm* pTmNow;
	pTmNow = localtime_r(&now, &tmNow);
	char buff[32] = { 0 };
	snprintf(buff, sizeof buff, "%04d-%02d-%02d %02d:%02d:%02d",
	         1900 + pTmNow->tm_year,
	         pTmNow->tm_mon + 1,
	         pTmNow->tm_mday,
	         pTmNow->tm_hour,
	         pTmNow->tm_min,
	         pTmNow->tm_sec);

	return std::string(buff);
}

const char* TimeUtility::GetStringTimeDetail()
{
	static char buff[64] = { 0 };
	static struct timeval tvNow;
	static time_t now;
	static struct tm tmNow;
	static struct tm* pTmNow;

	gettimeofday(&tvNow, NULL);
	now = static_cast<time_t>(tvNow.tv_sec);
	pTmNow = localtime_r(&now, &tmNow);

	snprintf(buff, sizeof buff, "%04d-%02d-%02d %02d:%02d:%02d.%06d",
	         1900 + pTmNow->tm_year,
	         pTmNow->tm_mon + 1,
	         pTmNow->tm_mday,
	         pTmNow->tm_hour,
	         pTmNow->tm_min,
	         pTmNow->tm_sec,
	         static_cast<int>(tvNow.tv_usec));

	return buff;
}

time_t TimeUtility::GetTimeStamp(const std::string& time)
{
	tm tm_;
	char buff[128] = { 0 };
	strncpy(buff, time.c_str(), sizeof(buff) - 1);
	buff[sizeof(buff) - 1] = 0;
	strptime(buff, "%Y-%m-%d %H:%M:%S", &tm_);
	tm_.tm_isdst = -1;
	return mktime(&tm_);
}

time_t TimeUtility::GetTimeDiff(const std::string& t1, const std::string& t2)
{
	time_t time1 = GetTimeStamp(t1);
	time_t time2 = GetTimeStamp(t2);
	return (time1 - time2);
}

} // end namespace common
