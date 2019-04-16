#ifndef __COMMON_THREAD_H__
#define __COMMON_THREAD_H__

#include <functional>
#include <memory>
#include <pthread.h>
#include <string>

#include "Atomic.h"

namespace common
{

class Thread : public noncopyable
{
public:
    typedef std::function<void()> ThreadFunc;

    explicit Thread(const ThreadFunc&, const std::string& name = std::string());
#ifdef __GXX_EXPERIMENTAL_CXX0X__
    explicit Thread(ThreadFunc&&, const std::string& name = std::string());
#endif

    ~Thread();

    void start();
    int join();

    bool started() const { return started_; }

    pid_t tid() const { return *tid_; }

    const std::string name() const { return name_; }

    static int numCreated() { return numCreated_.get(); }

private:
    void setDefaultName();

    bool started_;
    bool joined_;
    pthread_t pthreadId_;
    std::shared_ptr<pid_t> tid_;
    ThreadFunc func_;
    std::string name_;

    static AtomicInt32 numCreated_;
};

}//end namespace common

#endif //end __COMMON_THREAD_H__

