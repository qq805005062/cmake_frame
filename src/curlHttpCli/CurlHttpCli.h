#ifndef __XIAO_CURL_HTTP_CLI_H__
#define __XIAO_CURL_HTTP_CLI_H__

#include "src/Atomic.h"
#include "src/noncopyable.h"
#include "src/Singleton.h"
#include "src/AsyncCurlHttp.h"
#include "src/Thread.h"
#include "src/ThreadPool.h"
#include "HttpReqSession.h"

#define ABSOLUTELY_OUT_HTTP_SECOND				180

namespace CURL_HTTP_CLI
{

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

	//�����˳�ʱ���÷���������������첽����֮���˳��������������������ʾ�����Ѿ������
	void curlHttpCliExit();

	//��һ������ΪIO�߳������ڲ�������1���̣߳��ص�������һ���̴߳���,ͬʱ����ʱ����Ӧ�����ӣ����� 2
	//�ڶ������������Ķ��д�С���ڲ�������д�С��������ʱ�����һ�����У����Ϊ 0�������ƴ�С������Ƚϴ󣬶�����ڴ��ڳ����ٶȣ���Ӧ�ӳٿ��ܾͻ�ܴ�
	//��������������ÿ����������������ͬһ��ʱ�̣������г���������ֵ����Ӵ��ڡ�Ϊ 0 �����ƣ������һ�����Ӷ�ס�ˣ�Ӱ�������
	//���Ļ���������ÿ��ÿ���߳���ཨ�����ٸ����ӣ������������Ӱ��Ƚ����������������û�а취����ͬһ��ʱ����������������������Ӷ�ס�ˣ�����Ӱ����������󷢳�ȥ
	//		������ڱȽ��������Ļ����������ӳ��еȴ�̫�࣬����һ������Ҳ���������һ�����ӣ���������0 Ϊ������,
	//�����������͵��ĸ�����ֻ������һ����������ͬʱ����
	//����������������ٶȣ����������ô˽ӿڶ���ٶȡ���ֹ�ӳ�̫��0�ǲ�����
	//�������������ڲ�ͳ�ƴ��յ����󵽵��ûص��ĺ�ʱ���ᵥ������һ���߳��������־��Ϣ
	//���ó��ٻ������������ٶȸ����ϣ�����Ҳ������
	int curlHttpCliInit(int threadNum, int maxQueue, int maxPreConns = 0, int maxNewConns = 0, int maxSpeed = 0, int isShowTimeUse = 0);//���һ�������ڵ��Բ�������ʱ��Ż�ʹ��

	//�������������mallocʧ�ܷ���-1
	//��������Ѿ��յ��˳������-2
	//�˽ӿ�Ϊ�����ġ�������д�С���ˣ��ͻ�һֱ���������в����أ������޸�Ϊ���˾������˳���
	int curlHttpRequest(HttpReqSession& curlReq);

	////���淽���ڲ�ʹ�ã��ϲ�Ӧ�ò���ʹ�ã��������
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//�ڲ�ʹ�ã��ⲿʹ�ò���Ҫʹ��
	void curlHttpClientWakeup();

	void curlHttpThreadReady();

	uint32_t currentConnAdd()
	{
		return currentConn.incrementAndGet();
	}

	uint32_t currentConnDec()
	{
		return currentConn.decrementAndGet();
	}

	uint32_t curlHttpClientMaxConns()
	{
		uint32_t conns = 0;
		if(maxPreConns_ > 0)
		{
			conns = static_cast<uint32_t>(maxPreConns_);
		}
		return conns;
	}
	
private:

	void httpCliIoThread(int index);

	void httpStatisticsSecondThread();

	void httpRspCallBackThread();

	void httpOutRspCallBackThread();

	void curlHttpThreadExit();

	int ioThreadNum;
	int isExit_;
	int isReady;
	int isShowtime;
	int maxPreConns_;
	int maxNewConns_;
	AtomicUInt32 readyNum;
	AtomicUInt32 exitNum;
	AtomicUInt32 lastIndex;
	AtomicUInt32 currentConn;
	std::unique_ptr<ThreadPool> httpCliIoPoolPtr;
	std::unique_ptr<Thread> httpCliCallPtr;
	std::unique_ptr<Thread> httpCliOutCallPtr;
	std::unique_ptr<Thread> httpCliLog;
	std::vector<AsyncCurlHttp*> curlCliVect;
};

}
#endif