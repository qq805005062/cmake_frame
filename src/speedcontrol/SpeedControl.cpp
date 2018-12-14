
#include "Incommon.h"
#include "SpeedControl.h"
#include "ControlData.h"

namespace SPEED_CONTROL
{

DataStatistics::DataStatistics()
	:controlSpeedVec()
	,speedDataVec()
{
	PDEBUG("DataStatistics init");
}

DataStatistics::~DataStatistics()
{
	PERROR("~DataStatistics exit");
}

int DataStatistics::speedControlIndexInit(int index, int maxSpeed)
{
	int vectSize = static_cast<int>(controlSpeedVec.size());
	if(vectSize <= index)
	{
		vectSize = index - vectSize + 1;
	}
	for(int i = 0; i < vectSize; i++)
	{
		controlSpeedVec.push_back(nullptr);
	}

	controlSpeedVec[index].reset(new ControlSpeed(maxSpeed));

	if(controlSpeedVec[index] == nullptr)
	{
		return -1;
	}

	return 0;
}

int DataStatistics::speedDataIndexInit(int index)
{
	int vectSize = static_cast<int>(speedDataVec.size());
	if(vectSize <= index)
	{
		vectSize = index - vectSize + 1;
	}
	for(int i = 0; i < vectSize; i++)
	{
		speedDataVec.push_back(nullptr);
	}

	speedDataVec[index].reset(new SpeedData());
	if(speedDataVec[index] == nullptr)
	{
		return -1;
	}

	return 1;
}

void DataStatistics::speedDataAdd(int index)
{
	size_t vectIndex = static_cast<size_t>(index);
	if(speedDataVec.size() <= vectIndex)
	{
		return;
	}
	if(speedDataVec[vectIndex])
	{
		speedDataVec[vectIndex]->speedCount();
	}
}

int DataStatistics::controlSpeedAdd(int index)
{
	size_t vectIndex = static_cast<size_t>(index);
	if(controlSpeedVec.size() <= vectIndex)
	{
		return -1;
	}
	if(controlSpeedVec[vectIndex])
	{
		return controlSpeedVec[vectIndex]->speedCount();
	}
	return -1;
}

int DataStatistics::currentSpeed(int index, uint64_t *allnum)
{
	size_t vectIndex = static_cast<size_t>(index);
	if(speedDataVec.size() <= vectIndex)
	{
		return 0;
	}
	if(speedDataVec[vectIndex])
	{
		return speedDataVec[vectIndex]->everySecondSpeed(allnum);
	}
	return 0;
}

}
