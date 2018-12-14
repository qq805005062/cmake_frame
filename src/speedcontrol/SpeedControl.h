#ifndef __SPEED_CONTROL_H__
#define __SPEED_CONTROL_H__

#include <stdint.h>

#include "Singleton.h"
#include "noncopyable.h"
#include "ControlData.h"

namespace SPEED_CONTROL
{

/*
 *速度与控速模块，
 *此模块支持多个地方控制速度和计算速度
 *上层自己控制下标位置，不要重复，重复就会出问题，
 *控制速度和计数下标可以重复
 *上层必须要每隔一段时间去复位一下控速模块中的方法
 *每隔一段时间获取计数和总数方法
 *如果控速或者计算速度是单秒，就需要每隔一秒执行一次。
 *首先初始好各个模块，规划好各个下标
 *
 *上层自己控制好取的间隔
 *
 */
class DataStatistics : public noncopyable
{
public:
	DataStatistics();

	~DataStatistics();

	static DataStatistics& instance() { return Singleton<DataStatistics>::instance(); }

	//上层自己控制不要重复下标，一个模块控速一个下标
	int speedControlIndexInit(int index, int maxSpeed);

	//上层自己控制不要重复下标，一个模块计速度一个下标
	int speedDataIndexInit(int index);

	//模块速度每执行一次，调用一次
	void speedDataAdd(int index);

	//控制速度模块没执行一次，调用一次
	int controlSpeedAdd(int index);

	//每隔一段时间获取一次速度和总量
	int currentSpeed(int index, uint64_t *allnum);

#if 0
	//示例方法
	//这个方法上层必须每秒调用一次，复位速度控制的每个下标
	void everySecondStatistics()
	{
		int offset = 0;
		char speedInfo[2048] = {0}, *pChar = speedInfo;

		if(exit_)
		{
			return;
		}
		offset = sprintf(pChar, "\ntmt queue size %ld mt queue size %ld\n", TMtProtocolQueue::instance().queueSize(),  MtProtocolQueue::instance().queueSize());
		pChar += offset;
		for(size_t i = 0; i < SMSFWD::ConfigFile::instance().fwdsmsProtocolNum(); i++)
		{
			offset = sprintf(pChar, "mo queue index %ld size %ld\n", i, MoProQueuePtrVecObject::instance().moProQueuePtrVecQuSize(i));
			pChar += offset;
		}
		{
			uint64_t spdbSms = 0, spdbReport = 0, spdbUpsms = 0;
			int spdbSpeed = 0, spdbRepSpeed = 0, spdbUpsmsSpeed = 0;

			spdbSpeed = currentSpeed(SPDB_DOWN_SMS_SPEED_INDEX, &spdbSms);
			spdbRepSpeed = currentSpeed(SPDB_UP_REPORT_SPEED_INDEX, &spdbReport);
			spdbUpsmsSpeed = currentSpeed(SPDB_UP_SMS_SPEED_INDEX, &spdbUpsms);
			
			offset = sprintf(pChar, "spdb down sms %d::%ld up report %d::%ld up sms %d::%ld\n", spdbSpeed, spdbSms, spdbRepSpeed, spdbReport, spdbUpsmsSpeed, spdbUpsms);
			pChar += offset;
		}
		{
			uint64_t unionSms = 0, unionReport = 0, unionUpsms = 0;
			int unionSpeed = 0, unionRepSpeed = 0, unionUpsmsSpeed = 0;

			unionSpeed = currentSpeed(UNION_DOWN_SMS_SPEED_INDEX, &unionSms);
			unionRepSpeed = currentSpeed(UNION_UP_REPORT_SPEED_INDEX, &unionReport);
			unionUpsmsSpeed = currentSpeed(UNION_UP_SMS_SPEED_INDEX, &unionUpsms);
			
			offset = sprintf(pChar, "union down sms %d::%ld up report %d::%ld up sms %d::%ld\n-----------------------", unionSpeed, unionSms, unionRepSpeed, unionReport, unionUpsmsSpeed, unionUpsms);
			pChar += offset;
		}
		SPEED_LOG_INFO("%s", speedInfo);
	}
#endif
private:

	ControlSpeedPtrVector controlSpeedVec;
	SpeedDataPtrVect speedDataVec;
	
};

}
#endif