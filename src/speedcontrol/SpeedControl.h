#ifndef __SPEED_CONTROL_H__
#define __SPEED_CONTROL_H__

#include <stdint.h>

#include "Singleton.h"
#include "noncopyable.h"
#include "ControlData.h"

namespace SPEED_CONTROL
{

/*
 *�ٶ������ģ�飬
 *��ģ��֧�ֶ���ط������ٶȺͼ����ٶ�
 *�ϲ��Լ������±�λ�ã���Ҫ�ظ����ظ��ͻ�����⣬
 *�����ٶȺͼ����±�����ظ�
 *�ϲ����Ҫÿ��һ��ʱ��ȥ��λһ�¿���ģ���еķ���
 *ÿ��һ��ʱ���ȡ��������������
 *������ٻ��߼����ٶ��ǵ��룬����Ҫÿ��һ��ִ��һ�Ρ�
 *���ȳ�ʼ�ø���ģ�飬�滮�ø����±�
 *
 *�ϲ��Լ����ƺ�ȡ�ļ��
 *
 */
class DataStatistics : public noncopyable
{
public:
	DataStatistics();

	~DataStatistics();

	static DataStatistics& instance() { return Singleton<DataStatistics>::instance(); }

	//�ϲ��Լ����Ʋ�Ҫ�ظ��±꣬һ��ģ�����һ���±�
	int speedControlIndexInit(int index, int maxSpeed);

	//�ϲ��Լ����Ʋ�Ҫ�ظ��±꣬һ��ģ����ٶ�һ���±�
	int speedDataIndexInit(int index);

	//ģ���ٶ�ÿִ��һ�Σ�����һ��
	void speedDataAdd(int index);

	//�����ٶ�ģ��ûִ��һ�Σ�����һ��
	int controlSpeedAdd(int index);

	//ÿ��һ��ʱ���ȡһ���ٶȺ�����
	int currentSpeed(int index, uint64_t *allnum);

#if 0
	//ʾ������
	//��������ϲ����ÿ�����һ�Σ���λ�ٶȿ��Ƶ�ÿ���±�
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