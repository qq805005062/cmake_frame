#ifndef __LIBEVENT_TCP_CLIENT_H__
#define __LIBEVENT_TCP_CLIENT_H__

#include <mutex>

#include <stdint.h>
#include <functional>

#include "src/Singleton.h"
#include "src/noncopyable.h"

//#include "LibeventTcpCli.h"

namespace LIBEVENT_TCP_CLI
{

typedef std::function<void(uint64_t uniqueid, void* priv, bool isConn, const std::string& ipaddr, int port)> TcpConnectCallback;
//IO�̲߳�Ҫ���������������췵��
	
//�����Ѿ�ʹ�û�������������
typedef std::function<size_t(uint64_t uniqueid, void* priv, const char* msg, size_t msglen, const std::string& ipaddr, int port)> TcpOnMessageCallback;
//IO�̲߳�Ҫ������ʽ���������췵��

class LibeventTcpCli : public noncopyable
{
public:

	LibeventTcpCli();

	~LibeventTcpCli();

	static LibeventTcpCli& instance() { return Singleton<LibeventTcpCli>::instance();}

	void libeventTcpCliExit();

	int libeventTcpCliInit(unsigned int uniquId, unsigned int threadNum, const TcpConnectCallback& connCb_, const TcpOnMessageCallback& msgCb);

	int libeventAddConnect(const std::string& ipaddr, int port, void* priv = NULL, int outSecond = 30);

	int libeventTcpCliSendMsg(uint64_t unid, const char* msg, size_t msglen);

	//�ᴥ���ص������ǲ���IO�̻߳ص���Ҳ�������ɲ������ص�
	int libeventTcpCliDisconnect(uint64_t unid);
	
///�ڲ�ʹ�ã��ⲿ����Ҫ����
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void libeventIoThread(size_t index);

	void libeventIoThreadReady();

	void tcpServerConnect(uint64_t uniqueid, void* priv, bool isConn, const std::string& ipaddr, int port);

	size_t tcpServerOnMessage(uint64_t uniqueid, void* priv, const char* msg, size_t msglen, const std::string& ipaddr, int port);
		
private:
	int isExit;

	unsigned int uniquId_;
	unsigned int ioThreadNum;
	unsigned int lastIndex;
	unsigned int readyIothread;
	unsigned int exitIothread;

	TcpConnectCallback connCb;
	TcpOnMessageCallback msgCb;

	std::mutex mutex_;
};

}

#endif
