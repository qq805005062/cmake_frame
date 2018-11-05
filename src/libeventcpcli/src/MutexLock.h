#ifndef __LIBEVENT_TCPCLI_MUTEXLOCK_H__
#define __LIBEVENT_TCPCLI_MUTEXLOCK_H__

#include <assert.h>
#include <pthread.h>

#include "CurrentThread.h"
#include "noncopyable.h"

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       assert(errnum == 0); (void) errnum;})

namespace LIBEVENT_TCP_CLI
{

class MutexLock : noncopyable
{
public:
    MutexLock()
        : holder_(0)
    {
        MCHECK(pthread_mutex_init(&mutex_, NULL));
    }

    ~MutexLock()
    {
        assert(holder_ == 0);
        MCHECK(pthread_mutex_destroy(&mutex_));
    }

    bool isLockedByThisThread() const
    {
        return holder_ == CurrentThread::tid();
    }

    void assertLocked() const
    {
        assert(isLockedByThisThread());
    }

    void lock()
    {
        MCHECK(pthread_mutex_lock(&mutex_));
        assignHolder();
    }

    void unlock()
    {
        unassignHolder();
        MCHECK(pthread_mutex_unlock(&mutex_));
    }

    pthread_mutex_t* getPthreadMutex()
    {
        return &mutex_;
    }

private:
    friend class Condition;

    class UnassignGuard : noncopyable
    {
    public:
        UnassignGuard(MutexLock& owner)
            : owner_(owner)
        {
            owner_.unassignHolder();
        }
        ~UnassignGuard()
        {
            owner_.assignHolder();
        }

    private:
        MutexLock& owner_;
    };

    void unassignHolder()
    {
        holder_ = 0;
    }

    void assignHolder()
    {
        holder_ = CurrentThread::tid();
    }
    
private:
    pthread_mutex_t mutex_;
    pid_t holder_;
};

class SafeMutexLock : noncopyable
{
public:
    explicit SafeMutexLock(MutexLock& mutex) : mutex_(mutex)
    {
        mutex_.lock();
    }

    ~SafeMutexLock()
    {
        mutex_.unlock();
    }

private:
    MutexLock& mutex_;
};

} // namespace LIBEVENT_TCP_CLI

// Prevent misuse like:
// SafeMutexLock(mutex_);
// A tempory object doesn't hold the lock for long!
#define SafeMutexLock(x) error "Missing guard object name"

#endif  // __LIBEVENT_TCPCLI_MUTEXLOCK_H__

