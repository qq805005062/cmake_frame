#include <sys/time.h>
#include <stdio.h>

#include <inttypes.h>

#include "Timestamp.h"

namespace ZOOKEEPERKAFKA
{

static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp is same size as int64_t");

std::string Timestamp::toString() const
{
	char buf[32] = { 0 };
	int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
	// int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
	// snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
	//snprintf(buf, sizeof(buf) - 1, "%" PRId64 "", seconds);
	snprintf(buf, sizeof(buf) - 1, "%lu", seconds);
	return buf;
}

std::string Timestamp::toFormattedString(bool showMilliseconds) const
{
	char buf[32] = { 0 };
	time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
	struct tm tm_time;
	// gmtime_r(&seconds, &tm_time);
	localtime_r(&seconds, &tm_time);

	if (showMilliseconds)
	{
		int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
		snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
		         tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
		         tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
		         microseconds / 1000);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
		         tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
		         tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
	}
	return buf;
}

std::string Timestamp::FormattedString() const
{
	char buf[32] = { 0 };
	time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
	struct tm tm_time;
	// gmtime_r(&seconds, &tm_time);
	localtime_r(&seconds, &tm_time);

	int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
	snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d:%03d",
	         tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
	         tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
	         microseconds / 1000);
	
	return buf;
}

Timestamp Timestamp::now()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int64_t seconds = tv.tv_sec;
	return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

} // end namespace ZOOKEEPERKAFKA