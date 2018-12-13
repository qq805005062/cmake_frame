
#include <map>

#include "ThreadPool.h"
#include "Incommon.h"
#include "UniqueNum.h"
#include "TcpClient.h"
#include "LibeventIo.h"
#include "../LibeventTcpCli.h"

namespace LIBEVENT_TCP_CLI
{
typedef std::map<uint64_t, TcpClientPtr> TcpClientConnMap;
typedef TcpClientConnMap::iterator TcpClientConnMapIter;

static TcpClientConnMap tcpClientConnMap;
static std::unique_ptr<ThreadPool> eventIoPoolPtr;
static std::vector<LibeventIoPtr> libeventIoPtrVect;

LibeventTcpCli::LibeventTcpCli()
	:isExit(0)
	,uniquId_(0)
	,ioThreadNum(0)
	,lastIndex(0)
	,readyIothread(0)
	,exitIothread(0)
	,connCb(nullptr)
	,msgCb(nullptr)
	,mutex_()
{
	DEBUG("LibeventTcpCli init");
}

LibeventTcpCli::~LibeventTcpCli()
{
	ERROR("~LibeventTcpCli exit");
}

void LibeventTcpCli::libeventTcpCliExit()
{
	isExit = 1;

	DEBUG("libeventTcpCliExit");

	while(ioThreadNum == 0)
	{
		usleep(1000);
	}

	for(size_t i = 0; i < ioThreadNum; i++)
	{
		if(libeventIoPtrVect[i])
		{
			libeventIoPtrVect[i]->libeventIoExit();
		}
	}

	while(exitIothread != ioThreadNum)
	{
		usleep(1000);
	}

	for(size_t i = 0; i < ioThreadNum; i++)
	{
		if(libeventIoPtrVect[i])
		{
			libeventIoPtrVect[i].reset();
		}
	}

	if(eventIoPoolPtr)
	{
		eventIoPoolPtr->stop();
		eventIoPoolPtr.reset();
	}
}

int LibeventTcpCli::libeventTcpCliInit(unsigned int uniquId, unsigned int threadNum, const TcpConnectCallback& connCb_, const TcpOnMessageCallback& msgCb_)
{
	INFO("libventTcpCliInit init");

	if(ioThreadNum > 0)
	{
		INFO("libeventTcpCliInit had been init already");
		return 0;
	}
	
	eventIoPoolPtr.reset(new ThreadPool("eventIo"));
	if(eventIoPoolPtr == nullptr)
	{
		WARN("libeventTcpCliInit thread pool new error");
		return -1;
	}
	
	libeventIoPtrVect.resize(threadNum);
	eventIoPoolPtr->start(threadNum);
	for(size_t i = 0; i < threadNum; i++)
	{
		libeventIoPtrVect[i].reset();
		eventIoPoolPtr->run(std::bind(&LIBEVENT_TCP_CLI::LibeventTcpCli::libeventIoThread, this, i));
	}

	connCb = connCb_;
	msgCb = msgCb_;
	uniquId_ = uniquId;
	ioThreadNum = threadNum;
	return 0;
}

int LibeventTcpCli::libeventAddConnect(const std::string& ipaddr, int port, void* priv, int outSecond)
{
	if(isExit)
	{
		WARN("LibeventTcpCli had been exit");
		return -2;
	}
	
	while(ioThreadNum == 0 || readyIothread != ioThreadNum)
	{
		usleep(1000);
	}
	
	do{
		size_t index = 0;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			index = lastIndex++;
		}
		index = static_cast<int>(index % ioThreadNum);
		
		uint64_t uuid = uniqueNumId(uniquId_);
		DEBUG("uniqueNumId %lu", uuid);
		TcpClientPtr client(new TcpClient(index, uuid, ipaddr, port, priv, outSecond));
		if(client)
		{
			if(libeventIoPtrVect[index])
			{
				OrderNodePtr node(new OrderNode(client));
				if(node)
				{
					libeventIoPtrVect[index]->libeventIoOrder(node);
					{
						std::lock_guard<std::mutex> lock(mutex_);
						tcpClientConnMap.insert(TcpClientConnMap::value_type(uuid , client));
					}
					break;
				}else{
					WARN("libeventAddConnect new order node nullptr");
				}
			}else{
				WARN("libeventAddConnect libeventIoOrder error index %ld", index);
			}
		}else{
			WARN("libeventAddConnect new client node nullptr");
		}
	}while(1);
	return 0;
}

int LibeventTcpCli::libeventTcpCliSendMsg(uint64_t unid, const char* msg, size_t msglen)
{
	if(isExit)
	{
		WARN("LibeventTcpCli had been exit");
		return -2;
	}
	
	while(ioThreadNum == 0 || readyIothread != ioThreadNum)
	{
		usleep(1000);
	}

	TcpClientPtr client(nullptr);
	{
		std::lock_guard<std::mutex> lock(mutex_);
		TcpClientConnMapIter iter = tcpClientConnMap.find(unid);
		if(iter == tcpClientConnMap.end())
		{
			return -3;
		}
		client = iter->second;
	}

	if(client->isKeepAlive())
	{
		OrderNodePtr node(new OrderNode(client, msg, msglen));
		if(node)
		{
			if(libeventIoPtrVect[client->inIoThreadIndex()])
			{
				DEBUG("send msg");
				libeventIoPtrVect[client->inIoThreadIndex()]->libeventIoOrder(node);
				return 0;
			}else{
				WARN("libeventTcpCliSendMsg libeventIoOrder error index %ld", client->inIoThreadIndex());
				return -1;
			}
		}else{
			WARN("libeventTcpCliSendMsg new order node nullptr ");
			return -1;
		}
	}else{
		WARN("libeventTcpCliSendMsg conn had been exprie and will be disconnect %lu", unid);
		client->disConnect();
		LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(client->tcpCliUniqueNum(), client->tcpClientPrivate(), DIS_CONNECT, client->tcpServerIp(), client->tcpServerPort());
		return -3;
	}
	
	return 0;
}

int LibeventTcpCli::libeventTcpCliResetPrivate(uint64_t unid, void* priv)
{
	if(isExit)
	{
		WARN("LibeventTcpCli had been exit");
		return -2;
	}
	
	while(ioThreadNum == 0 || readyIothread != ioThreadNum)
	{
		usleep(1000);
	}

	TcpClientPtr client(nullptr);
	{
		std::lock_guard<std::mutex> lock(mutex_);
		TcpClientConnMapIter iter = tcpClientConnMap.find(unid);
		if(iter == tcpClientConnMap.end())
		{
			return -3;
		}
		client = iter->second;
	}

	client->setTcpCliPrivate(priv);
	return 0;
}

int LibeventTcpCli::libeventTcpCliDisconnect(uint64_t unid)
{
	if(isExit)
	{
		WARN("LibeventTcpCli had been exit");
		return -2;
	}
	
	while(ioThreadNum == 0 || readyIothread != ioThreadNum)
	{
		usleep(1000);
	}

	TcpClientPtr client(nullptr);
	{
		std::lock_guard<std::mutex> lock(mutex_);
		TcpClientConnMapIter iter = tcpClientConnMap.find(unid);
		if(iter == tcpClientConnMap.end())
		{
			return -3;
		}
		client = iter->second;
	}

	OrderNodePtr node(new OrderNode(client));
	if(node)
	{
		if(libeventIoPtrVect[client->inIoThreadIndex()])
		{
			libeventIoPtrVect[client->inIoThreadIndex()]->libeventIoOrder(node);
			return 0;
		}else{
			WARN("libeventTcpCliDisconnect libeventIoOrder error index %ld", client->inIoThreadIndex());
			return -1;
		}
	}else{
		WARN("libeventTcpCliDisconnect new order node nullptr ");
		return -1;
	}

	if(client->tcpCliState() == CONN_FAILED)
	{
		LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(client->tcpCliUniqueNum(), client->tcpClientPrivate(), CONN_FAILED, client->tcpServerIp(), client->tcpServerPort());
	}else{
		LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(client->tcpCliUniqueNum(), client->tcpClientPrivate(), DIS_CONNECT, client->tcpServerIp(), client->tcpServerPort());
	}
	return 0;
}

void LibeventTcpCli::libeventIoThreadReady()
{
	std::lock_guard<std::mutex> lock(mutex_);
	readyIothread++;
}

void LibeventTcpCli::libeventIoThread(size_t index)
{
	int ret = 0;
	while(1)
	{
		libeventIoPtrVect[index].reset(new LibeventIo());
		if(libeventIoPtrVect[index] == nullptr)
		{
			WARN("LibeventTcpCli new io Thread object error");
			continue;
		}
		ret = libeventIoPtrVect[index]->libeventIoReady();
		{
			std::lock_guard<std::mutex> lock(mutex_);
			readyIothread--;
		}
		INFO("LibeventIoPtrVect[index]->asyncCurlReady ret %d", ret);
		if(isExit)
		{
			break;
		}
	}
	{
		std::lock_guard<std::mutex> lock(mutex_);
		exitIothread++;
	}
	return;
}

void LibeventTcpCli::tcpServerConnect(uint64_t unid, void* priv, int state, const std::string& ipaddr, int port)
{
	DEBUG("tcpServerConnect");

	{
		std::lock_guard<std::mutex> lock(mutex_);
		TcpClientConnMapIter iter = tcpClientConnMap.find(unid);
		if(iter == tcpClientConnMap.end())
		{
			return;
		}
		if(state != CONN_SUCCESS)
		{
			tcpClientConnMap.erase(iter);
		}
	}	if(connCb)
	{
		connCb(unid, priv, state, ipaddr, port);
	}
		
}

size_t LibeventTcpCli::tcpServerOnMessage(uint64_t unid, void* priv, const char* msg, size_t msglen, const std::string& ipaddr, int port)
{
	if(msgCb)
	{
		return msgCb(unid, priv, msg, msglen, ipaddr, port);
	}
	return msglen;
}


}

