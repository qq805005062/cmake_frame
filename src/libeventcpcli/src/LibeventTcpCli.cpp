
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
}

int LibeventTcpCli::libeventTcpCliInit(unsigned int uniquId, unsigned int threadNum, const TcpConnectCallback& connCb_, const TcpOnMessageCallback& msgCb)
{
	INFO("libventTcpCliInit init");
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
				libeventIoPtrVect[client->inIoThreadIndex()]->libeventIoOrder(node);
				return 0;
			}else{
				return -1;
			}
		}else{
			return -1;
		}
	}else{
		client->disConnect();
		LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(client->tcpCliUniqueNum(), client->tpcClientPrivate(), false, client->tcpServerIp(), client->tcpServerPort());
		return -3;
	}
	
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

	client->disConnect();
	LIBEVENT_TCP_CLI::LibeventTcpCli::instance().tcpServerConnect(client->tcpCliUniqueNum(), client->tpcClientPrivate(), false, client->tcpServerIp(), client->tcpServerPort());
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
		libeventIoPtrVect[index]->libeventIoReady();
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
}

void LibeventTcpCli::tcpServerConnect(uint64_t unid, void* priv, bool isConn, const std::string& ipaddr, int port)
{
	if(connCb)
	{
		connCb(unid, priv, isConn, ipaddr, port);
	}

	if(isConn == false)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		TcpClientConnMapIter iter = tcpClientConnMap.find(unid);
		if(iter == tcpClientConnMap.end())
		{
			return;
		}
		tcpClientConnMap.erase(iter);
	}
}

size_t LibeventTcpCli::tcpServerOnMessage(uint64_t unid, void* priv, const char* msg, size_t msglen, const std::string& ipaddr, int port)
{
	if(msgCb)
	{
		return msgCb(unid, priv, msg, msglen, ipaddr, port);
	}
	return 0;
}


}

