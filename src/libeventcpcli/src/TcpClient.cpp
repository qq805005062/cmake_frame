
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

static void connectTimeout(evutil_socket_t fd, short event, void *arg)
{
    TcpClient *pClient = static_cast<TcpClient*>(arg);
    if(pClient->tcpCliState() == CONN_FAILED)
    {
        pClient->disConnect();
        LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), CONN_FAILED, pClient->tcpServerIp(), pClient->tcpServerPort());
    }
}

static void TcpClientOnMessage(struct bufferevent *bev, void *arg)
{
    TcpClient *pClient = static_cast<TcpClient*>(arg);
    int second = pClient->setRecvSecond();
    if(second < 0)
    {
        ERROR("TcpClientOnMessage :: %s:%d outtime will be disconnect", pClient->tcpServerIp(), pClient->tcpServerPort());
        pClient->disConnect();
        LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), DIS_CONNECT, pClient->tcpServerIp(), pClient->tcpServerPort());
        return;
    }

    struct evbuffer* msgBuf= bufferevent_get_input(bev);
    size_t allLen = evbuffer_get_length(msgBuf);
    char *resMsg = new char[allLen];
    if(resMsg)
    {
        size_t ret = evbuffer_copyout(msgBuf, resMsg, allLen);
        if(ret == allLen)
        {
            ret = LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerOnMessage(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), resMsg, allLen, pClient->tcpServerIp(), pClient->tcpServerPort());
            int result = evbuffer_drain(msgBuf, ret);
            if(result < 0)
            {
                ERROR("TcpClientOnMessage :: %s:%d evbuffer_drain result %d", pClient->tcpServerIp(), pClient->tcpServerPort(), result);
                pClient->disConnect();
                LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), DIS_CONNECT, pClient->tcpServerIp(), pClient->tcpServerPort());
            }
        }else{
            ERROR("TcpClientOnMessage :: %s:%d evbuffer_copyout ret %ld", pClient->tcpServerIp(), pClient->tcpServerPort(), ret);
            pClient->disConnect();
            LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), DIS_CONNECT, pClient->tcpServerIp(), pClient->tcpServerPort());
        }
        delete[] resMsg;
    }else{
        ERROR("TcpClientOnMessage :: %s:%d new recv msg buff nullptr", pClient->tcpServerIp(), pClient->tcpServerPort());
        pClient->disConnect();
        LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), DIS_CONNECT, pClient->tcpServerIp(), pClient->tcpServerPort());
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
        INFO("TcpClientOnEvent :: %s:%d  %lu connection closed", pClient->tcpServerIp(), pClient->tcpServerPort(), pClient->tcpCliUniqueNum());
        pClient->disConnect();
        if(pClient->tcpCliState() == CONN_FAILED)
        {
            LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), CONN_FAILED, pClient->tcpServerIp(), pClient->tcpServerPort());
        }else{
            LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), DIS_CONNECT, pClient->tcpServerIp(), pClient->tcpServerPort());
        }
    }
    else if (event & BEV_EVENT_ERROR)
    {
        INFO("TcpClientOnEvent :: %s:%d  %lu some other error connection closed", pClient->tcpServerIp(), pClient->tcpServerPort(), pClient->tcpCliUniqueNum());
        pClient->disConnect();
        if(pClient->tcpCliState() == CONN_FAILED)
        {
            LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), CONN_FAILED, pClient->tcpServerIp(), pClient->tcpServerPort());	
        }else{
            LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), DIS_CONNECT, pClient->tcpServerIp(), pClient->tcpServerPort());
        }
    }
    else if( event & BEV_EVENT_CONNECTED)  
    {
        pClient->tcpClieSetconn();
        INFO("TcpClientOnEvent :: %s:%d  %lu had connected to server", pClient->tcpServerIp(), pClient->tcpServerPort(), pClient->tcpCliUniqueNum());   
        LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), 1, pClient->tcpServerIp(), pClient->tcpServerPort());
    }else{
        INFO("TcpClientOnEvent :: %s:%d  %lu %d some other error connection closed", pClient->tcpServerIp(), pClient->tcpServerPort(), pClient->tcpCliUniqueNum(), event);
        pClient->disConnect();
        if(pClient->tcpCliState() == CONN_FAILED)
        {
            LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), CONN_FAILED, pClient->tcpServerIp(), pClient->tcpServerPort());	
        }else{
            LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(pClient->tcpCliUniqueNum(), pClient->tcpClientPrivate(), DIS_CONNECT, pClient->tcpServerIp(), pClient->tcpServerPort());
        }
    }
}

TcpClient::TcpClient(size_t ioIndex, uint64_t uniqueNum, const std::string& ipaddr, int port, void* priv, int dataOutSecond, int connOutSecond)
    :port_(port)
    ,sockfd_(0)
    ,connOutSecond_(connOutSecond)
    ,dataOutSecond_(dataOutSecond)
    ,state_(CONN_FAILED)
    ,ioIndex_(ioIndex)
    ,outbufLen_(0)
    ,uniqueNum_(uniqueNum)
    ,lastRecvSecond_(0)
    ,priv_(priv)
    ,timev_(nullptr)
    ,base_(nullptr)
    ,bev_(nullptr)
    ,ipaddr_(ipaddr)
{
    DEBUG("TcpClient init");
    lastRecvSecond_ = secondSinceEpoch();
}

TcpClient::~TcpClient()
{
	ERROR("~TcpClient exit");

	priv_ = NULL;
	base_ = NULL;
	if(timev_)
	{
		event_free(timev_);
		timev_ = nullptr;
	}
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
	
	timev_ = evtimer_new(base_, connectTimeout, static_cast<void*>(this));
	if(timev_ == nullptr)
	{
		ERROR("evtimer_new malloc null");
		return -1;
	}
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

	struct timeval read_tv = { dataOutSecond_, 0 };
	struct timeval write_tv = { dataOutSecond_, 0 };
	bufferevent_set_timeouts(bev_, &read_tv, &write_tv);

	memset(&server_addr, 0, sizeof(server_addr) );  
    server_addr.sin_family = AF_INET;  
    server_addr.sin_port = htons(port_);  
    inet_aton(ipaddr_.c_str(), &server_addr.sin_addr);

	struct timeval connSecondOut = {connOutSecond_, 0};
	evtimer_add(timev_, &connSecondOut);// call back outtime check
	ret = bufferevent_socket_connect(bev_, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(ret < 0)
	{
		ERROR("TcpClient bufferevent_socket_connect failed : %p  %d", base_, ret);
		event_free(timev_);
		timev_ = nullptr;
		return ret;
	}
	lastRecvSecond_ = secondSinceEpoch();
	if(bev_)
	{
		sockfd_ = bufferevent_getfd(bev_);
	}else{
		ERROR("TcpClient bufferevent_socket_connect failed : %p  %d", base_, ret);
		return -1;
	}
	DEBUG("connectServer ip port sockfd %s:%d %d  %lu", ipaddr_.c_str(), port_, sockfd_, uniqueNum_);
	return sockfd_;
}

void TcpClient::disConnect()
{
	if(timev_)
	{
		event_free(timev_);
		timev_ = nullptr;
	}
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

bool TcpClient::isKeepAlive(uint64_t second)
{
	bool result = true;
	if(lastRecvSecond_  && ((second - lastRecvSecond_) > dataOutSecond_))
	{
		ERROR("tcp cli ip port sockfd %s %d %d %ld had been expire", ipaddr_.c_str(), port_, sockfd_, outbufLen_);
		result = false;
	}
	return result;
}

int TcpClient::setRecvSecond()
{
	uint64_t second = secondSinceEpoch();

	if(lastRecvSecond_  && ((second - lastRecvSecond_) > dataOutSecond_))
	{
		return -1;
	}

	lastRecvSecond_ = second;
	return 0;
}
}