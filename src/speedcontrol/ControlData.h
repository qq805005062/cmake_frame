#ifndef __SPEED_CONTROL_DATA_H__
#define __SPEED_CONTROL_DATA_H__

#include <stdint.h>
#include <mutex>
#include <vector>
#include <memory>

#include "Atomic.h"
#include "Singleton.h"
#include "noncopyable.h"

namespace SPEED_CONTROL
{

typedef AtomicIntegerT<uint64_t> AtomicUInt64;

class ControlSpeed
{
public:
	ControlSpeed(int maxSpeed)
		:maxSpeed_(0)
		,lastCounNum_(0)
		,allCountNum_()
	{
		PDEBUG("ControlSpeed init");
		maxSpeed_ = static_cast<uint64_t>(maxSpeed);
	}

	~ControlSpeed()
	{
		PERROR("~ControlSpeed exit");
	}

	int speedCount()
	{
		uint64_t nowCount = allCountNum_.incrementAndGet();
		uint64_t nowSpeed = nowCount - lastCounNum_;
		if(maxSpeed_ && (nowSpeed > maxSpeed_))
		{
			return -1;
		}
		return 0;
	}

	//每秒必须要调用一次，不然会影响控速模块
	void everySeondReset()
	{
		lastCounNum_ = allCountNum_.get();
	}
private:
	uint64_t maxSpeed_;
	uint64_t lastCounNum_;
	AtomicUInt64 allCountNum_;
	
};

typedef std::shared_ptr<ControlSpeed> ControlSpeedPtr;
typedef std::vector<ControlSpeedPtr> ControlSpeedPtrVector;

class SpeedData
{
public:
	SpeedData()
		:lastCounNum_(0)
		,allCountNum_()
	{
		PDEBUG("SpeedData init");
	}

	~SpeedData()
	{
		PERROR("~SpeedData init");
	}

	void speedCount()
	{
		allCountNum_.increment();
	}

	//每秒钟获取总数和速度
	int everySecondSpeed(uint64_t* allnum)
	{
		uint64_t all = allCountNum_.get();
		if(allnum)
		{
			*allnum = all;
		}
		int speed = static_cast<int>(all - lastCounNum_);
		lastCounNum_ = all;
		return speed;
	}
	
private:
	uint64_t lastCounNum_;
	AtomicUInt64 allCountNum_;
};

typedef std::shared_ptr<SpeedData> SpeedDataPtr;
typedef std::vector<SpeedDataPtr> SpeedDataPtrVect;

}
#endif
