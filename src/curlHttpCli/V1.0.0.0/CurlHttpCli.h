#ifndef __XIAO_CURL_HTTP_CLI_H__
#define __XIAO_CURL_HTTP_CLI_H__

#include <mutex>

#include "src/noncopyable.h"
#include "src/Singleton.h"

#include "HttpReqSession.h"

#define CURL_HTTP_CLI_VERSION			"V1.0.0.0"//ģ��汾��

#define ABSOLUTELY_OUT_HTTP_SECOND		180//���Եĳ�ʱʱ�䣬���ٶȹ����ʱ��libcurl��libevent�г�ʱ���ƿ���û�а취������������Գ�ʱʱ��Ὣ��Щ�ص����ã���ֹ������
#define MAX_SIMULTANEOUSLY_CONNS		10000//Ĭ���������ӣ����Բ����Գ���������

namespace CURL_HTTP_CLI
{
/*
 *Http�ͻ��ˣ��κ�һ�����̣�ȫ�ֵ�����
 *ÿ�������Ӧһ���ص�����ʼ������session��(һ��Ҫ���ûص�)���ص���������������Լ���Ӧ����
 *������ͷ�ļ�����ʼ�����߳���������������������д�СΪ0���������ƣ��ڴ������ǣ�����ֵ�ˣ�����ӿ�����������˻�����
 *
 *�˰汾��֧������һ��URL���߶��URL��ֻ�е���URLʱ�ſ�������Ϊ�����ӣ���������URLʱ��һ�������Գ����ӣ�
 *�����������ȫ�����õģ�����������ٸ���ͬ��URL
 *
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
	//Ĭ�Ϲ��캯��
	CurlHttpCli();

	//Ĭ����������
	~CurlHttpCli();

	static CurlHttpCli& instance() { return Singleton<CurlHttpCli>::instance();}

	//�����˳�ʱ���÷���������������첽����֮���˳��������������������ʾ�����Ѿ�����ˣ��������ݽṹ�Ѿ�ȫ������
	void curlHttpCliExit();

	//��һ������ΪIO�߳������ڲ��������1���̣߳��ص�������һ���̴߳���,ͬʱ������ʱ����Ӧ�����ӣ����� 2
	//�ڶ������������Ķ��д�С���ڲ�������д�С��������ʱ�����һ�����У����Ϊ 0�������ƴ�С������Ƚϴ󣬶�����ڴ��ڳ����ٶȣ���Ӧ�ӳٿ��ܾͻ�ܴ�
	//�������������Ƿ񱣳ֳ����ӣ�������ģ��ֻ����һ��URL,�ſ�������Ϊ���ֳ����ӣ������Ҫ������URL,�����������Ϊ0
	//���ĸ�������ͬʱ��������ά�����ӳصĴ�С��Ĭ������ܳ���10000��
	//��������������٣����ô˽ӿڲ����Գ�������ٶȣ������ٶ�ʹ��(��ʱ��û��ʹ��)
	int curlHttpCliInit(unsigned int threadNum, unsigned int maxQueue, unsigned int isKeepAlive = 0, unsigned int maxConns = MAX_SIMULTANEOUSLY_CONNS,unsigned int maxSpeed = 0);

	//�������������mallocʧ�ܷ���-1
	//��������Ѿ��յ��˳������-2
	//�˽ӿ�Ϊ�����ġ�������д�С���ˣ��ͻ�һֱ���������в����أ������޸�Ϊ���˾������˳���
	int curlHttpRequest(HttpReqSession& curlReq);

	////���淽���ڲ�ʹ�ã��ϲ�Ӧ�ò���ʹ�ã��������
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//�ڲ�ʹ�ã��ⲿʹ�ò���Ҫʹ��
	void curlHttpClientWakeup();

	void curlHttpThreadReady();

	unsigned int httpIsKeepAlive()
	{
		return isKeepAlive;
	}
private:

	void httpCliIoThread(size_t index);

	void httpRspCallBackThread();

	void httpIoWakeThread();

	int isExit;
	int threadExit;
	unsigned int readyIothread;
	unsigned int exitIothread;
	unsigned int isKeepAlive;
	unsigned int lastIndex;
	unsigned int ioThreadNum;
	unsigned int ioMaxConns;
	
	std::mutex mutex_;
};

}

#endif