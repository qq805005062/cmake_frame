#include <assert.h>
#include <stdio.h>

#include "ThreadPool.h"
#include "Exception.h"

namespace ZOOKEEPERKAFKA
{

ThreadPool::ThreadPool(const std::string& name)
	: mutex_(),
	  notEmpty_(mutex_),
	  notFull_(mutex_),
	  name_(name),
	  maxQueueSize_(0),
	  running_(false)
{
}

ThreadPool::~ThreadPool()
{
	if (running_)
	{
		stop();
	}
}

void ThreadPool::start(int numThreads)
{
	assert(threads_.empty());
	running_ = true;
	threads_.reserve(numThreads);
	for (int i = 0; i < numThreads; ++i)
	{
		int ret = 0;
		char id[32];
		ret = snprintf(id, sizeof id, "%d", i+1);
		id[ret] = '\0';
		threads_.emplace_back(new Thread(std::bind(&ThreadPool::runInThread, this), name_+id));
		threads_[i]->start();
	}

	if (numThreads == 0 && threadInitCallback_)
	{
		threadInitCallback_();
	}
}

void ThreadPool::stop()
{
	{
	MutexLockGuard lock(mutex_);
	running_ = false;
	notEmpty_.notifyAll();
	}
	for (auto& thr : threads_)
	{
		thr->join();
	}
}

size_t ThreadPool::queueSize() const
{
	MutexLockGuard lock(mutex_);
	return queue_.size();
}

void ThreadPool::run(const Task& task)
{
	if (threads_.empty())
	{
		task();
	}
	else
	{
		MutexLockGuard lock(mutex_);
		while (isFull())
		{
			notFull_.wait();
		}
		assert(!isFull());

		queue_.push_back(task);
		notEmpty_.notify();
	}
}

bool ThreadPool::TryRun(const Task& task)
{
  if (threads_.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    if (isFull())
    {
      return false;
    }
    assert(!isFull());

    queue_.push_back(task);
    notEmpty_.notify();
  }
  return true;
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__DONOT_USE
void ThreadPool::run(Task&& task)
{
  if (threads_.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    while (isFull())
    {
      notFull_.wait();
    }
    assert(!isFull());

    queue_.push_back(std::move(task));
    notEmpty_.notify();
  }
}

bool ThreadPool::TryRun(Task&& task)
{
  if (threads_.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    if (isFull())
    {
      return false;
    }
    assert(!isFull());

    queue_.push_back(std::move(task));
    notEmpty_.notify();
  }
  return true;
}

#endif

ThreadPool::Task ThreadPool::take()
{
	MutexLockGuard lock(mutex_);
	while (queue_.empty() && running_)
	{
		notEmpty_.wait();
	}

	Task task;
	if (!queue_.empty())
	{
		task = queue_.front();
		queue_.pop_front();
		if (maxQueueSize_ > 0)
		{
			notFull_.notify();
		}
	}
	return task;
}

bool ThreadPool::isFull() const
{
	mutex_.assertLocked();
	return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
	try
	{
		if (threadInitCallback_)
		{
			threadInitCallback_();
		}
		while (running_)
		{
			Task task(take());
			if (task)
			{
				task();
			}
		}
	}
	catch (const Exception& ex)
	{
		fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
		fprintf(stderr, "reason: %s\n", ex.what());
		fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
		abort();
	}
	catch (const std::exception& ex)
	{
		fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
		fprintf(stderr, "reason: %s\n", ex.what());
		abort();
	}
	catch (...)
	{
		fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
		throw;
	}
}

} // end namespace ZOOKEEPERKAFKA