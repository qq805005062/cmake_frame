#ifndef __COMMON_THREADLOCAL_SINGLETON_H__
#define __COMMON_THREADLOCAL_SINGLETON_H__

#include <common/noncopyable.h>

#include <assert.h>
#include <pthread.h>

namespace common
{

template<typename T>
class ThreadLocalSingleton : noncopyable
{
public:
	static T& instance()
	{
		if (!t_value_)
		{
			t_value_ = new T();
			deleter_.set(t_value_);
		}
		return *t_value_;
	}

	static T* pointer()
	{
		return t_value_;
	}

private:
	ThreadLocalSingleton();
	~ThreadLocalSingleton();

	static void destructor(void* obj)
	{
		assert(obj == t_value_);
		typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
		T_must_be_complete_type dummy; (void) dummy;
		delete t_value_;
		t_value_ = 0;
	}

	class Deleter
	{
	public:
		Deleter()
		{
			pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
		}

		~Deleter()
		{
			pthread_key_delete(pkey_);
		}

		void set(T* newObj)
		{
			assert(pthread_getspecific(pkey_) == NULL);
			pthread_setspecific(pkey_, newObj);
		}

		pthread_key_t pkey_;
	};

	static __thread T* t_value_;
	static Deleter deleter_;
};

template<typename T>
__thread T* ThreadLocalSingleton<T>::t_value_ = 0;

template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

} // end namespace common

#endif // __COMMON_THREADLOCAL_SINGLETON_H__
