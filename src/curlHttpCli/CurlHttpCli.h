#ifndef __XIAO_CURL_HTTP_CLI_H__
#define __XIAO_CURL_HTTP_CLI_H__

#include "src/Atomic.h"
#include "src/noncopyable.h"
#include "src/Singleton.h"
#include "src/AsyncCurlHttp.h"
#include "src/ThreadPool.h"
#include "HttpReqSession.h"

namespace CURL_HTTP_CLI
{

typedef AtomicIntegerT<uint32_t> AtomicUInt32;

/*
 *Http�ͻ��ˣ��κ�һ�����̣�ȫ�ֵ�����
 *ÿ���������һ���ص�����ʼ������session��(һ��Ҫ���ûص�)���ص���������������Լ���Ӧ����
 *������ͷ�ļ�����ʼ�����߳���������������������д�СΪ0���������ƣ��ڴ������ǣ�����ֵ�ˣ�����ӿ�����������˻�����
 *
 *ʹ�÷���:
 *�� curlHttpCliInit ֮�� curlHttpRequest���ӿ����첽�ģ��˳�ʱһ��Ҫ���� curlHttpCliExit,
 *curlHttpCliExit�������ģ�������˵�����������Ѿ�����
 *����ÿ����������ӳ�ʱʱ������ݳ�ʱʱ�䶼��һ�������ܻ�curlHttpCliExit�������ĳ�ʱʱ��
 *
 */
class CurlHttpCli : public noncopyable
{
public:

	CurlHttpCli();

	~CurlHttpCli();

	static CurlHttpCli& instance() { return Singleton<CurlHttpCli>::instance();}

	void curlHttpCliExit();

	int curlHttpCliInit(int threadNum, int maxQueue, int isShowTimeUse = 0);//���һ�������ڵ��Բ�������ʱ��Ż�ʹ��

	int curlHttpRequest(HttpReqSession& curlReq);//��������������������ˣ���һֱ����

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//�ڲ�ʹ�ã��ⲿʹ�ò���Ҫʹ��
	void curlHttpClientWakeup();

	void curlHttpThreadReady();

	int64_t curlHttpEventSeq()
	{
		return testEventSeq.incrementAndGet();
	}

private:

	void httpIoThreadFun(int index);

	void httpStatisticsSecond();

	int threadNum_;
	int isExit_;
	int isReady;
	int isShowtime;
	AtomicUInt32 readyNum;
	AtomicUInt32 lastIndex;
	AtomicInt64  testEventSeq;
	std::unique_ptr<ThreadPool> httpCliPoolPtr;
	std::vector<AsyncCurlHttp*> curlCliVect;
};

}
#endif