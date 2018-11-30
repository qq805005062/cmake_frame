
#include "Incommon.h"
#include "SpeedControl.h"
#include "ControlData.h"

namespace SPEED_CONTROL
{

SpeedControl::SpeedControl()
	:spdNum()
	,spdCtlNum()
{
	PDEBUG("SpeedControl init");
}

SpeedControl::~SpeedControl()
{
	PERROR("~SpeedControl exit");
}

int SpeedControl::spdItfaceIden()
{
	int ret = 0;
	uint32_t num = spdNum.incrementAndGet();
	ret = SpeedDataVector::instance().speedDataVectorAdd(num);
	if(ret < 0)
	{
		return ret;
	}
	return static_cast<int>(num);
}

int SpeedControl::spdCtlItfaceIden(uint32_t maxSpeed)
{
	int ret = 0;
	uint32_t num = spdCtlNum.incrementAndGet();
	ret = ControlSpeedVector::instance().controlSpeedVctAdd(num, maxSpeed);
	if(ret < 0)
	{
		return ret;
	}
	return static_cast<int>(num);
}

int SpeedControl::speedConunt(int iden)
{
	uint32_t num = static_cast<uint32_t>(iden);
	return SpeedDataVector::instance().speedDataAdd(num);
}

int SpeedControl::speedControl(int iden)
{
	uint32_t num = static_cast<uint32_t>(iden);
	return ControlSpeedVector::instance().controlSpeedAdd(num);
}

}
