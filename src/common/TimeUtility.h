#pragma once

#include <string>

namespace common
{

class TimeUtility
{
public:
	// 得到当前的毫秒
	static int64_t GetCurrentMS();

	// 得到当前的微秒
	static int64_t GetCurrentUS();

	// 得到字符串形式的时间 格式: yyyy-mm-dd HH:MI:SS
	static std::string GetStringTime();

	// 得到字符串形式的详细时间，格式: yyyy-mm-dd HH:MI:SS.xxxxxx
	static const char* GetStringTimeDetail();

	// 将字符串格式(yyyy-mm-dd HH:MI:SS)的时间，转化为time_t(时间戳)
	static time_t GetTimeStamp(const std::string& time);

	// 取得两个时间戳字符串t1-t2的时间差,精确到秒,时间格式为:yyyy-mm-dd HH:MI:SS
	static time_t GetTimeDiff(const std::string& t1, const std::string& t2);
};

} // end namespace common
