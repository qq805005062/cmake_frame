#ifndef __SPEED_CONTROL_H__
#define __SPEED_CONTROL_H__

#include <stdint.h>

#include "Singleton.h"
#include "noncopyable.h"
#include "Atomic.h"

namespace SPEED_CONTROL
{

typedef AtomicIntegerT<uint32_t> AtomicUInt32;
/*
 *�����ٶȺͿ���ģ��
 *
 *��ģ��
 *���Լ������һ���ӿڵ�������ٶ�
 *
 *���Կ���һ���ӿڵ����������ٶ�
 *
 *ʹ�÷��������ȵ���spdItfaceIden ����spdCtlItfaceIden Ϊÿ�����ٻ��߿��ٽӿ�����Ψһһ����ţ�����ֵС��0Ϊ����
 *����Ψһ���֮��ÿ�ε��ýӿڵ���speedConunt ����speedControl����
 *
 *����ӿ�С��һ��һ�εĻ����Ͳ������ٶ�
 */
class SpeedControl : public noncopyable
{
public:
	SpeedControl();

	~SpeedControl();

	static SpeedControl& instance() { return Singleton<SpeedControl>::instance();}

	//����ֵС��0���ڲ�malloc����
	//�����ٶ�������Žӿ�
	int spdItfaceIden();

	//����˽ӿ�����ٶ�
	//����ֵС��0���ڲ�malloc����
	int spdCtlItfaceIden(uint32_t maxSpeed);

	//���ص���0��δͳ���ٶȣ��˽ӿ�û��һ��᷵��һ���ٶ�ֵ������0.
	//����-2���ڲ�δ��ʼ����Ӧ������ʶ
	int speedConunt(int iden);

	//����ֵ-1�ǳ���
	//����ֵ-2���ڲ�δ��ʼ��
	int speedControl(int iden);
	
private:
	
	AtomicUInt32 spdNum;
	AtomicUInt32 spdCtlNum;
};

}
#endif