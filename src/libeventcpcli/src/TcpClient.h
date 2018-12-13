#ifndef __LIBEVENT_TCPCLI_CLIENT_H__
#define __LIBEVENT_TCPCLI_CLIENT_H__

#include <event.h>
//#include <event2/event.h>
#include <event2/bufferevent.h>  
#include <event2/buffer.h>  
//#include <event2/util.h> 

//#include "TcpClient.h"
namespace LIBEVENT_TCP_CLI
{

class TcpClient
{
public:
	//io线程下标，唯一数字标识，ip地址，端口，私有信息，超时时间，
	TcpClient(size_t ioIndex, uint64_t uniqueNum, const std::string& ipaddr, int port, void* priv, int outSecond = 30);

	~TcpClient();

	int connectServer(struct event_base* eBase);

	void disConnect();

	void sendMsg(const void* msg, size_t len);

	void* tcpClientPrivate()
	{
		return priv_;
	}

	void setTcpCliPrivate(void* p)
	{
		priv_ = p;
	}

	int isKeepAlive();

	uint64_t tcpCliUniqueNum()
	{
		return uniqueNum_;
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	size_t inIoThreadIndex()
	{
		return ioIndex_;
	}

	void setRecvSecond();

	const char* tcpServerIp() const
	{
		return ipaddr_.c_str();
	}

	int tcpServerPort() const
	{
		return port_;
	}

	void setReadySendbufLen(size_t buflen)
	{
		outbufLen_ = buflen;
	}

	int tcpCliSockFd()
	{
		return sockfd_;
	}

	int tcpCliState()
	{
		return state_;
	}

	void tcpClieSetconn()
	{
		if(timev_)
		{
			evtimer_del(timev_);
			timev_ = nullptr;
		}
		
state_ = CONN_SUCCESS;
	}
	
private:
	int port_;
	int sockfd_;
	int outSecond_;
	int state_;// 0是初始状态，未连接成功， 1 是连接成功， 2是连接成功之后断开
	
	size_t ioIndex_;
	size_t outbufLen_;

	uint64_t uniqueNum_;
	uint64_t lastRecvSecond_;
	void* priv_;
	struct event *timev_;
	struct event_base *base_;
	struct bufferevent *bev_;

	std::string ipaddr_;
};

typedef std::shared_ptr<TcpClient> TcpClientPtr;
}
#endif
