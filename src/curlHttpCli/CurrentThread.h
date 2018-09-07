#ifndef __CURLCURRENT_THREAD_H__
#define __CURLCURRENT_THREAD_H__

#include <stdint.h>
namespace CURLHTTPCLI
{
namespace CurrentThread
{

	__thread int  t_cachedTid = 0;
	__thread char t_tidString[32];
	__thread int  t_tidStringLength = 6;
	__thread const char* t_threadName = "unknown";
	static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

	/*
	extern __thread int t_cachedTid;
	extern __thread char t_tidString[32];
	extern __thread int t_tidStringLength;
	extern __thread const char* t_threadName;
	*/

	void cacheTid()
	{
		if (t_cachedTid == 0)
		{
			t_cachedTid = gettid();
			t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d", t_cachedTid);
		}
	}

	inline int tid()
	{
		if (__builtin_expect(t_cachedTid == 0, 0))
		{
			cacheTid();
		}
		return t_cachedTid;
	}

	inline const char* tidString()
	{
		return t_tidString;
	}

	inline int tidStringLength()
	{
		return t_tidStringLength;
	}

	inline const char* name()
	{
		return t_threadName;
	}

	bool isMainThread()
	{
		return tid() == ::getpid();
	}

	void sleepUsec(int64_t usec);
} // end namespace CurrentThread
} // end namespace common

#endif // __COMMON_CURRENT_THREAD_H__
