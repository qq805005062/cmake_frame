#ifndef __LIBEVENT_TCPCLI_ATOMIC_H__
#define __LIBEVENT_TCPCLI_ATOMIC_H__

#include <stdint.h>

#include "noncopyable.h"

namespace LIBEVENT_TCP_CLI
{

template<typename T>
class AtomicIntegerT : public noncopyable
{
 public:
  AtomicIntegerT()
	: value_(0)
  {
  }

  // uncomment if you need copying and assignment
  //
  // AtomicIntegerT(const AtomicIntegerT& that)
  //   : value_(that.get())
  // {}
  //
  // AtomicIntegerT& operator=(const AtomicIntegerT& that)
  // {
  //   getAndSet(that.get());
  //   return *this;
  // }

  T get()
  {
	// in gcc >= 4.7: __atomic_load_n(&value_, __ATOMIC_SEQ_CST)
	return __sync_val_compare_and_swap(&value_, 0, 0);
  }

  T getAndAdd(T x)
  {
	// in gcc >= 4.7: __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST)
	return __sync_fetch_and_add(&value_, x);
  }

  T addAndGet(T x)
  {
	return getAndAdd(x) + x;
  }

  T incrementAndGet()
  {
	return addAndGet(1);
  }

  T decrementAndGet()
  {
	return addAndGet(-1);
  }

  void add(T x)
  {
	getAndAdd(x);
  }

  void increment()
  {
	incrementAndGet();
  }

  void decrement()
  {
	decrementAndGet();
  }

  T getAndSet(T newValue)
  {
	// in gcc >= 4.7: __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST)
	return __sync_lock_test_and_set(&value_, newValue);
  }

 private:
  volatile T value_;
};

typedef AtomicIntegerT<int32_t> AtomicInt32;
typedef AtomicIntegerT<int64_t> AtomicInt64;
typedef AtomicIntegerT<uint32_t> AtomicUInt32;



}

#endif// end of __LIBEVENT_TCPCLI_ATOMIC_H__

