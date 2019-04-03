#ifndef __COMMON_MUTEXLOCK_H__
#define __COMMON_MUTEXLOCK_H__

#include <assert.h>
#include <pthread.h>

#include <common/CurrentThread.h>
#include <common/noncopyable.h>

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       assert(errnum == 0); (void) errnum;})

namespace common
{

class MutexLock : noncopyable
{
public:
    MutexLock()
        : holder_(0)
        , isLock(0)
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

    int trylock()
    {
        int ret = pthread_mutex_trylock(&mutex_);
        if(ret == 0)
        {
            isLock = 1;
            assignHolder();
        }
        return ret;
    }

    void tryUnlock()
    {
        unassignHolder();
        if(isLock)
        {
            isLock = 0;
            pthread_mutex_unlock(&mutex_);
        }
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
    int isLock;
};

class MutexLockGuard : noncopyable
{
public:
    explicit MutexLockGuard(MutexLock& mutex) : mutex_(mutex)
    {
        mutex_.lock();
    }

    ~MutexLockGuard()
    {
        mutex_.unlock();
    }

private:
    MutexLock& mutex_;
};

class TryMutexLock : noncopyable
{
public:
    explicit TryMutexLock(MutexLock& mutex) : mutex_(mutex), isLock(0)
    {
        isLock = mutex_.trylock();
    }

    ~TryMutexLock()
    {
        mutex_.tryUnlock();
    }

    bool isLockMutex()
    {
        if(isLock)
        {
            return false;
        
}else{
            return true;
        }
    }

private:
    MutexLock& mutex_;
    volatile int isLock;
};
} // namespace common

// Prevent misuse like:
// MutexLockGuard(mutex_);
// A tempory object doesn't hold the lock for long!
#define MutexLockGuard(x) error "Missing guard object name"

#endif  // __COMMON_MUTEXLOCK_H__
