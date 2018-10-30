#ifndef __XIAO_CURL_HTTP_CLI_H__
#define __XIAO_CURL_HTTP_CLI_H__

#include <mutex>

#include "src/noncopyable.h"
#include "src/Singleton.h"

#include "HttpReqSession.h"


#define ABSOLUTELY_OUT_HTTP_SECOND		180
#define MAX_SIMULTANEOUSLY_CONNS		10000

namespace CURL_HTTP_CLI
{

class CurlHttpCli : public noncopyable
{
public:

	CurlHttpCli();

	~CurlHttpCli();

	static CurlHttpCli& instance() { return Singleton<CurlHttpCli>::instance();}

	//�����˳�ʱ���÷���������������첽����֮���˳��������������������ʾ�����Ѿ������
	void curlHttpCliExit();

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

	size_t httpIoThreadNum()
	{
		size_t num = static_cast<size_t>(ioThreadNum);
		return num;
	}

	unsigned int httpIsKeepAlive()
	{
		return isKeepAlive;
	}
private:

	void httpCliIoThread(size_t index);

	void httpRspCallBackThread();

	void httpOutRspCallBackThread();

	int isExit;
	int threadExit;
	unsigned int readyIothread;
	unsigned int exitIothread;
	unsigned int isKeepAlive;
	unsigned int lastIndex;
	unsigned int ioThreadNum;
	
	std::mutex mutex_;
};

}

#endif
