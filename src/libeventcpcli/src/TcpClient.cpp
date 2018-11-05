
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  

#include <memory.h>

#include "Incommon.h"
#include "TcpClient.h"
#include "../LibeventTcpCli.h"

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-compare"
namespace LIBEVENT_TCP_CLI
{

static void TcpClientOnMessage(struct bufferevent *bev, void *arg)
{
	TcpClient *pClient = static_cast<TcpClient*>(arg);
	pClient->setRecvSecond();
	
	struct evbuffer* msgBuf= bufferevent_get_input(bev);
	size_t allLen = evbuffer_get_length(msgBuf);
	char *resMsg = new char[allLen];
	if(resMsg)
	{
		size_t ret = evbuffer_copyout(msgBuf, resMsg, allLen);
		if(ret == allLen)
		{
			ret = LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerOnMessage(pClient->tcpCliUniqueNum(), pClient->tpcClientPrivate(), resMsg, allLen, pClient->tcpServerIp(), pClient->tcpServerPort());
			int result = evbuffer_drain(msgBuf, ret);
			if(result < 0)
			{
				ERROR("TcpClientOnMessage :: %s:%d evbuffer_drain result %d", pClient->tcpServerIp(), pClient->tcpServerPort(), result);
				pClient->disConnect();
				LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tpcClientPrivate(), false, pClient->tcpServerIp(), pClient->tcpServerPort());
			}
		}else{
			ERROR("TcpClientOnMessage :: %s:%d evbuffer_copyout ret %ld", pClient->tcpServerIp(), pClient->tcpServerPort(), ret);
			pClient->disConnect();
			LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tpcClientPrivate(), false, pClient->tcpServerIp(), pClient->tcpServerPort());
		}
		delete[] resMsg;
	}else{
		ERROR("TcpClientOnMessage :: %s:%d new recv msg buff nullptr", pClient->tcpServerIp(), pClient->tcpServerPort());
		pClient->disConnect();
		LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tpcClientPrivate(), false, pClient->tcpServerIp(), pClient->tcpServerPort());
	}
}

static void TcpClientWriteBack(struct bufferevent *bev, void *arg)
{
	TcpClient *pClient = static_cast<TcpClient*>(arg);
	struct evbuffer* msgBuf= bufferevent_get_output(bev);
	size_t allLen = evbuffer_get_length(msgBuf);
	pClient->setReadySendbufLen(allLen);
}

static void TcpClientOnEvent(struct bufferevent *bev, short event, void *arg)
{
	TcpClient *pClient = static_cast<TcpClient*>(arg);
	if (event & BEV_EVENT_EOF)
	{
		INFO("TcpClientOnEvent :: %s:%d connection closed", pClient->tcpServerIp(), pClient->tcpServerPort());
		pClient->disConnect();
		LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tpcClientPrivate(), false, pClient->tcpServerIp(), pClient->tcpServerPort());
	}
    else if (event & BEV_EVENT_ERROR)
    {
		INFO("TcpClientOnEvent :: %s:%d some other error connection closed", pClient->tcpServerIp(), pClient->tcpServerPort());
		pClient->disConnect();
		LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tpcClientPrivate(), false, pClient->tcpServerIp(), pClient->tcpServerPort());
    }
    else if( event & BEV_EVENT_CONNECTED)  
    {
		INFO("TcpClientOnEvent :: %s:%d had connected to server", pClient->tcpServerIp(), pClient->tcpServerPort());   
		LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tpcClientPrivate(), true, pClient->tcpServerIp(), pClient->tcpServerPort());
    }
}

TcpClient::TcpClient(size_t ioIndex, uint64_t uniqueNum, const std::string& ipaddr, int port, void* priv, int outSecond)
	:port_(port)
	,sockfd_(0)
	,outSecond_(outSecond)
	,ioIndex_(ioIndex)
	,outbufLen_(0)
	,uniqueNum_(uniqueNum)
	,lastRecvSecond_(0)
	,priv_(priv)
	,base_(nullptr)
	,bev_(nullptr)
	,ipaddr_(ipaddr)
{
	DEBUG("TcpClient init");
}

TcpClient::~TcpClient()
{
	ERROR("~TcpClient exit");

	priv_ = NULL;
	base_ = NULL;
	if(bev_)
	{
		bufferevent_free(bev_);
		bev_ = NULL;
	}
	ipaddr_.assign("");
}

int TcpClient::connectServer(struct event_base* eBase)
{
	int ret = 0;
	struct sockaddr_in server_addr;
	if(eBase == NULL || ipaddr_.empty() || port_ == 0)
	{
		ERROR("connectServer parameter error");
		return -3;
	}

	base_ = eBase;
	bev_ = bufferevent_socket_new(base_, -1, BEV_OPT_CLOSE_ON_FREE);
	if(bev_ == NULL)
	{
		ERROR("bufferevent_socket_new malloc null");
		return -1;
	}

	//bufferevent_setwatermark(bev_, EV_READ | EV_WRITE, 10, 1024 * 600);
	//bufferevent_setwatermark(bev_, EV_WRITE, 10, 1024 * 600);
	bufferevent_setcb(bev_, TcpClientOnMessage, TcpClientWriteBack, TcpClientOnEvent, static_cast<void*>(this));  
    //bufferevent_enable(bev_, EV_READ | EV_PERSIST | EV_ET);
	ret = bufferevent_enable(bev_, EV_READ | EV_PERSIST );
	if(ret < 0)
	{
		ERROR("TcpClient bufferevent_enable failed : %p %d", base_, ret);
		return ret;
	}

	const struct timeval read_tv = { outSecond_, 0 };
	const struct timeval write_tv = { outSecond_, 0 };
	bufferevent_set_timeouts(bev_, &read_tv, &write_tv);

	memset(&server_addr, 0, sizeof(server_addr) );  
    server_addr.sin_family = AF_INET;  
    server_addr.sin_port = htons(port_);  
    inet_aton(ipaddr_.c_str(), &server_addr.sin_addr);
	ret = bufferevent_socket_connect(bev_, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(ret < 0)
	{
		ERROR("TcpClient bufferevent_socket_connect failed : %p  %d", base_, ret);
		return ret;
	}
	lastRecvSecond_ = secondSinceEpoch();
	sockfd_ = bufferevent_getfd(bev_);
	DEBUG("connectServer ip port sockfd %s %d %d", ipaddr_.c_str(), port_, sockfd_);
	return sockfd_;
}

void TcpClient::disConnect()
{
	if(bev_)
	{
		bufferevent_free(bev_);
		bev_ = NULL;
	}
}

void TcpClient::sendMsg(const void* msg, size_t len)
{
	if(bev_)
	{
		bufferevent_write(bev_, msg, len);
	}else{
		ERROR("tcp cli ip port sockfd had been disconnect %s %d %d ", ipaddr_.c_str(), port_, sockfd_);
	}
}

int TcpClient::isKeepAlive()
{
	int ret = 1;
	uint64_t second = secondSinceEpoch();
	DEBUG("tcp cli ip port sockfd %s %d %d %ld", ipaddr_.c_str(), port_, sockfd_, outbufLen_);
	if((second - lastRecvSecond_) > outSecond_)
	{
		ERROR("tcp cli ip port sockfd %s %d %d %ld had been expire", ipaddr_.c_str(), port_, sockfd_, outbufLen_);
		return 0;
	}
	return ret;
}

void TcpClient::setRecvSecond()
{
	lastRecvSecond_ = secondSinceEpoch();
}
}