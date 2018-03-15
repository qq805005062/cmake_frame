#ifndef __COMMON_COUNTDOWNLATCH_H__
#define __COMMON_COUNTDOWNLATCH_H__

#include <common/MutexLock.h>
#include <common/Condition.h>

namespace common
{

class CountDownLatch : noncopyable
{
public:
	explicit CountDownLatch(int count);

	void wait();

	void countDown();

	int getCount() const;

private:
	mutable MutexLock mutex_;
	Condition condition_;
	int count_;
};

} // end namespace common

#endif // __COMMON_COUNTDOWNLATCH_H__
