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
 *计算速度和控速模块
 *
 *此模块
 *可以计算输出一个接口单秒调用速度
 *
 *可以控制一个接口单秒最大调用速度
 *
 *使用方法，首先调用spdItfaceIden 或者spdCtlItfaceIden 为每个计速或者控速接口生产唯一一个编号，返回值小于0为错误
 *生产唯一编号之后，每次调用接口调用speedConunt 或者speedControl即可
 *
 *如果接口小于一秒一次的话，就不会有速度
 */
class SpeedControl : public noncopyable
{
public:
	SpeedControl();

	~SpeedControl();

	static SpeedControl& instance() { return Singleton<SpeedControl>::instance();}

	//返回值小于0是内部malloc错误
	//计算速度生产编号接口
	int spdItfaceIden();

	//传入此接口最大速度
	//返回值小于0是内部malloc错误
	int spdCtlItfaceIden(uint32_t maxSpeed);

	//返回等于0是未统计速度，此接口没隔一秒会返回一次速度值，大于0.
	//返回-2是内部未初始化对应参数标识
	int speedConunt(int iden);

	//返回值-1是超速
	//返回值-2是内部未初始化
	int speedControl(int iden);
	
private:
	
	AtomicUInt32 spdNum;
	AtomicUInt32 spdCtlNum;
};

}
#endif