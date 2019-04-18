#ifndef MW_ATOMICINT_H
#define MW_ATOMICINT_H

#include <stdint.h>

namespace MWATOMICINT
{
template<typename T> 
/*T only support:int8_t,int16_t,int32_t,int64_t,uint8_t,uint16_t,uint32_t,uint64_t*/
class AtomicIntT
{
public:
	AtomicIntT();

	// ���ص�ǰֵ
	T	Get();

	// �ȷ��ص�ǰֵ,Ȼ���ټ���x,����i++
	T	GetAndAdd(T x);

	// �ȼ���x,�ٷ��ؼӺ��ֵ,����++i
	T 	AddAndGet(T x);

	// �ȼ���1,�ٷ��ؼӺ��ֵ
	T 	IncAndGet();

	// �ȼ�ȥ1,�ٷ��ؼ����ֵ
	T 	DecAndGet();

	// ����x,������ֵ,������x++
	void Add(T x);

	// ����1,������ֵ,������x++
	void Inc();

	// ��ȥ1,������ֵ,������x--
	void Dec();

	// �ȷ��ص�ǰֵ,Ȼ�������ó��µ�ֵnewValue
	T 	GetAndSet(T newValue);

private:
	volatile T value_;
};

/////////////////////////////////////////////////////////////////////////////
template<typename T> 
AtomicIntT<T>::AtomicIntT()
	: value_(0)
{
}

template<typename T> 
T AtomicIntT<T>::Get()
{
	// in gcc >= 4.7: __atomic_load_n(&value_, __ATOMIC_SEQ_CST)
	return __sync_val_compare_and_swap(&value_, 0, 0);
}

template<typename T> 
T AtomicIntT<T>::GetAndAdd(T x)
{
	// in gcc >= 4.7: __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST)
	return __sync_fetch_and_add(&value_, x);
}

template<typename T> 
T AtomicIntT<T>::AddAndGet(T x)
{
	return GetAndAdd(x) + x;
}

template<typename T> 
T AtomicIntT<T>::IncAndGet()
{
	return AddAndGet(1);
}

template<typename T> 
T AtomicIntT<T>::DecAndGet()
{
	return AddAndGet(-1);
}

template<typename T> 
void AtomicIntT<T>::Add(T x)
{
	GetAndAdd(x);
}

template<typename T> 
void AtomicIntT<T>::Inc()
{
	IncAndGet();
}

template<typename T> 
void AtomicIntT<T>::Dec()
{
	DecAndGet();
}

template<typename T> 
T AtomicIntT<T>::GetAndSet(T newValue)
{
	// in gcc >= 4.7: __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST)
	return __sync_lock_test_and_set(&value_, newValue);
}

///////////////////////////////////////////////////////////////////////////////////////////
typedef AtomicIntT<int8_t> AtomicInt8;	// not all support
typedef AtomicIntT<int16_t> AtomicInt16; // not all support
typedef AtomicIntT<int32_t> AtomicInt32; // support
typedef AtomicIntT<int64_t> AtomicInt64; // support
typedef AtomicIntT<uint8_t> AtomicUint8; // not all support
typedef AtomicIntT<uint16_t> AtomicUint16; // not all support
typedef AtomicIntT<uint32_t> AtomicUint32; // support
typedef AtomicIntT<uint64_t> AtomicUint64; // support
////////////////////////////////////////////////////////////////////////////////////////////
}
#endif


/*
����ʾ��:

using namespace MWATOMICINT;

int main(int argc, char* argv[])
{
	MWATOMICINT::AtomicInt32 test_int32;
	test_int32.Inc();
	printf("%d\n",test_int32.Get());
	
	while(1)
	{
		sleep(1);
	}
}
*/
