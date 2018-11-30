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

class SpeedData
{
public:
	SpeedData(uint32_t num)
		:moduleNum(num)
		,lastSecond(0)
		,lastSpeed(0)
		,speed()
		,mutex_()
	{
		PDEBUG("SpeedData init");
	}

	~SpeedData()
	{
		PERROR("~SpeedData exit");
	}

	int speedAdd()
	{
		uint64_t nowSpeed = speed.incrementAndGet();
		int64_t second = secondSinceEpoch();
		if(lastSecond == 0)
		{
			lastSecond = second;
			return 0;
		}

		if(second == lastSecond)
		{
			return 0;
		}

		std::lock_guard<std::mutex> lock(mutex_);
		if(second - lastSecond == 1)
		{
			uint64_t curr = nowSpeed - lastSpeed;
			PDEBUG("module num %d speed %ld", moduleNum, curr);
			lastSpeed = nowSpeed;
			lastSecond = second;
			return static_cast<int>(curr);
		}

		if(lastSecond != second)
		{
			lastSpeed = nowSpeed;
			lastSecond = second;
		}
		return 0;
	}

private:
	uint32_t moduleNum;
	volatile int64_t lastSecond;
	volatile uint64_t lastSpeed;
	AtomicUInt64 speed;
	std::mutex mutex_;
};
typedef std::shared_ptr<SpeedData> SpeedDataPtr;

class ControlSpeed
{
public:
	ControlSpeed(uint32_t num, uint32_t maxSpeed)
		:moduleNum(num)
		,lastSecond(0)
		,maxSpd(maxSpeed)
		,lastSpeed(0)
		,speed()
		,mutex_()
	{
		PDEBUG("ControlSpeed init");
	}

	~ControlSpeed()
	{
		PERROR("~ControlSpeed exit");
	}

	int controlAdd()
	{
		uint64_t nowSpeed = speed.incrementAndGet();
		int64_t second = secondSinceEpoch();
		if(lastSecond == 0)
		{
			lastSecond = second;
			if(nowSpeed > maxSpd)
			{
				return -1;
			}
			return 0;
		}
		
		if(second == lastSecond)
		{
			uint64_t curr = nowSpeed - lastSpeed;
			if(curr > maxSpd)
			{
				return -1;
			}
			return 0;
		}
		
		std::lock_guard<std::mutex> lock(mutex_);
		if(nowSpeed < lastSpeed)
		{
			lastSpeed = nowSpeed;
		}
		lastSecond = second; 
		return 0;
	}

private:
	uint32_t moduleNum;
	volatile int64_t lastSecond;
	uint64_t maxSpd;
	volatile uint64_t lastSpeed;
	AtomicUInt64 speed;
	std::mutex mutex_;

};
typedef std::shared_ptr<ControlSpeed> ControlSpeedPtr;

class SpeedDataVector : public noncopyable
{
public:
	SpeedDataVector()
		:SpeedDataPtrVect()
	{
		PDEBUG("SpeedDataVector init");
	}

	~SpeedDataVector()
	{
		PERROR("~SpeedDataVector exit");
	}

	static SpeedDataVector& instance() { return Singleton<SpeedDataVector>::instance();}

	int speedDataVectorAdd(uint32_t sunIndx)
	{
		while(SpeedDataPtrVect.size() < sunIndx)
		{
			SpeedDataPtrVect.push_back(nullptr);
		}

		SpeedDataPtr speed(new SpeedData(sunIndx));
		if(speed == nullptr)
		{
			return -1;
		}
		SpeedDataPtrVect.push_back(speed);
		return 0;
	}

	int speedDataAdd(uint32_t sunIndx)
	{
		if(SpeedDataPtrVect.size() > sunIndx && SpeedDataPtrVect[sunIndx])
		{
			return SpeedDataPtrVect[sunIndx]->speedAdd();
		}
		return -2;
	}

private:
	std::vector<SpeedDataPtr> SpeedDataPtrVect;
};

class ControlSpeedVector : public noncopyable
{
public:
	ControlSpeedVector()
		:ControlSpeedPtrVect()
	{
		PDEBUG("ControlSpeedVector init");
	}

	~ControlSpeedVector()
	{
		PERROR("~ControlSpeedVector exit");
	}

	static ControlSpeedVector& instance() { return Singleton<ControlSpeedVector>::instance();}

	int controlSpeedVctAdd(uint32_t sunIndx, uint32_t maxSpeed)
	{
		while(ControlSpeedPtrVect.size() < sunIndx)
		{
			ControlSpeedPtrVect.push_back(nullptr);
		}

		ControlSpeedPtr cSpeed(new ControlSpeed(sunIndx, maxSpeed));
		if(cSpeed == nullptr)
		{
			return -1;
		}
		ControlSpeedPtrVect.push_back(cSpeed);
		return 0;
	}

	int controlSpeedAdd(uint32_t sunIndx)
	{
		if(ControlSpeedPtrVect.size() > sunIndx && ControlSpeedPtrVect[sunIndx])
		{
			return ControlSpeedPtrVect[sunIndx]->controlAdd();
		}
		return -2;
	}
private:
	std::vector<ControlSpeedPtr> ControlSpeedPtrVect;
};

}
#endif
