
#ifndef __XIAO_NUMBER_ALIGN_H__
#define __XIAO_NUMBER_ALIGN_H__

#include "Atomic.h"
#include "noncopyable.h"
#include "Singleton.h"
#include "MutexLock.h"

//#include "NumberAlign.h"
namespace CURL_HTTP_CLI
{
class AsyncQueueNum : public noncopyable
{
public:
	AsyncQueueNum()
		:asyncNum()
	{
	}
	
	~AsyncQueueNum()
	{
	}

	static AsyncQueueNum& instance() { return Singleton<AsyncQueueNum>::instance();}

	void asyncReq()
	{
		asyncNum.increment();
	}

	void asyncRsp()
	{
		asyncNum.decrement();
	}

	int64_t asyncQueNum()
	{
		return asyncNum.get();
	}
private:
	
	AtomicInt64 asyncNum;
};

}

#endif

