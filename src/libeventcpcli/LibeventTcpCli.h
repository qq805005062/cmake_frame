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
//IO线程不要做阻塞操作，尽快返回
	
//返回已经使用缓冲区多少数据
typedef std::function<size_t(uint64_t uniqueid, void* priv, const char* msg, size_t msglen, const std::string& ipaddr, int port)> TcpOnMessageCallback;
//IO线程不要做阻塞式操作，尽快返回

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

	//会触发回调，但是不在IO线程回调，也可以做成不触发回调
	int libeventTcpCliDisconnect(uint64_t unid);
	
///内部使用，外部不需要关心
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
