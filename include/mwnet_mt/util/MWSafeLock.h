#ifndef MW_SAFELOCK_H
#define MW_SAFELOCK_H

#include <boost/any.hpp>

namespace MWSAFELOCK
{
	class BaseLock
	{
	public:
		BaseLock();
		~BaseLock();	
	public:
		void Lock();
		void UnLock();
	private:
		boost::any m_any;
	};

	class SafeLock
	{
	public:
  		explicit SafeLock(BaseLock& lock); // suggested
			explicit SafeLock(BaseLock* plock); // just compatible with old bussiness system
  		~SafeLock();
	private:
  		BaseLock* m_pLock;
	};
}
#endif 

/*
����ʾ��:

using namespace MWSAFELOCK;

//���붨������Ա������ȫ�ֱ���,��Ҫ�ٶ����ָ����,�ڴ���������ָ��
MWSAFELOCK::BaseLock lock; 
int main(int argc, char* argv[])
{
	{
		// lock....
		
		//���붨�����ʱ����,ÿ����Ҫ����ʱ,�Ա������Ƶ���̬����,��ʱ��������ʱ���Զ�����
		MWSAFELOCK::SafeLock safelock(lock); 
		//MWSAFELOCK::SafeLock safelock(&lock); 
		
		// do something......

		// no need unlock manually
	}

	// when safelock leave scope,auto destruct,auto unlock
		
	while(1)
	{
		sleep(1);
	}
}
*/
