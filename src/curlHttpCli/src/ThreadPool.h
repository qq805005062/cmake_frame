#ifndef __XIAO_THREAD_POOL_H__
#define __XIAO_THREAD_POOL_H__

#include <deque>
#include <vector>

#include "Condition.h"
#include "Thread.h"

namespace CURL_HTTP_CLI
{

class ThreadPool : noncopyable
{
public:
	typedef std::function<void ()> Task;

	explicit ThreadPool(const std::string& name = std::string("ThreadPool"));
	~ThreadPool();

	// Mutex be called before start().
	void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
	void setThreadInitCallback(const Task& cb)
	{ threadInitCallback_ = cb; }

	void start(int numThreads);
	void stop();

	const std::string& name() const
	{ return name_; }

	size_t queueSize() const;

	// Could block if maxQueueSize > 0
	void run(const Task& f);

  // don't block if full
  bool TryRun(const Task& f);
#ifdef __GXX_EXPERIMENTAL_CXX0X__DONOT_USE
  void run(Task&& f);
  // don't block if full
  bool TryRun(Task&& f);

#endif

private:
	bool isFull() const;
	void runInThread();
	Task take();

	mutable MutexLock mutex_;
	Condition notEmpty_;
	Condition notFull_;
	std::string name_;
	Task threadInitCallback_;
	std::vector<std::unique_ptr<Thread> > threads_;
	std::deque<Task> queue_;
	size_t maxQueueSize_;
	bool running_;
};

} // end namespace common

#endif

