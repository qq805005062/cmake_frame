#ifndef __XIAO_CONDITION_H__
#define __XIAO_CONDITION_H__

#include <pthread.h>

#include "MutexLock.h"

namespace CURL_HTTP_CLI
{

class Condition : noncopyable
{
public:
	explicit Condition(MutexLock& mutex)
		: mutex_(mutex)
	{
		MCHECK(pthread_cond_init(&pcond_, NULL));
	}

	~Condition()
	{
		MCHECK(pthread_cond_destroy(&pcond_));
	}

	void wait()
	{
		MutexLock::UnassignGuard ug(mutex_);
		MCHECK(pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()));
	}

	bool waitForSeconds(double seconds);

	void notify()
	{
		MCHECK(pthread_cond_signal(&pcond_));
	}

	void notifyAll()
	{
		MCHECK(pthread_cond_broadcast(&pcond_));
	}

private:
	MutexLock& mutex_;
	pthread_cond_t pcond_;
};

} // end namepsace CURL_HTTP_CLI

#endif // __XIAO_CONDITION_H__

