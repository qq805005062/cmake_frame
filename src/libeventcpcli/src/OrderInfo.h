#ifndef __LIBEVENT_TCPCLI_ORDERINFO_H__
#define __LIBEVENT_TCPCLI_ORDERINFO_H__

//#include "OrderInfo.h"

#include <deque>

#include "MutexLock.h"
#include "TcpClient.h"

namespace LIBEVENT_TCP_CLI
{

class OrderNode
{
public:
    OrderNode()
        :wriMsg()
        ,cli(nullptr)
    {
    }

    OrderNode(const TcpClientPtr& cl)
        :wriMsg()
        ,cli(cl)
    {
    }

    OrderNode(const TcpClientPtr& cl, const std::string& msg)
        :wriMsg(msg)
        ,cli(cl)
    {
    }

    OrderNode(const TcpClientPtr& cl, const char* msg, size_t msgLen)
        :wriMsg(msg, msgLen)
        ,cli(cl)
    {
    }

    OrderNode(const OrderNode& that)
        :wriMsg()
        ,cli(nullptr)
    {
        *this = that;
    }

    OrderNode& operator=(const OrderNode& that)
    {
        if(this == &that) return *this;

        wriMsg = that.wriMsg;
        cli = that.cli;

        return *this;
    }

    ~OrderNode()
    {
    }

    void setOrderNodeMsg(const std::string& msg)
    {
        wriMsg.assign(msg);
    }

    void setOrderNodeMsg(const char* msg, size_t msglen)
    {
        wriMsg.assign(msg, msglen);
    }

    void setOrderNodeTcpcli(const TcpClientPtr& cl)
    {
        cli = cl;
    }

    std::string orderNodeWrimsg()
    {
        return wriMsg;
    }

    TcpClientPtr orderNodeTcpcli()
    {
        return cli;
    }

private:

    std::string wriMsg;//根据下面的类判断连接，如果已经连接，但是string为空的话，则说明是要断连接
    TcpClientPtr cli;//可以根据类中标志位判断是否已经连接，如果未连接，则建议连接/
};

typedef std::shared_ptr<OrderNode> OrderNodePtr;
typedef std::deque<OrderNodePtr> OrderNodePtrDeque;

class OrderNodeDeque
{
public:

	OrderNodeDeque()
		:mutex_()
		,queue_()
	{
	}

	~OrderNodeDeque()
	{
	}

	void orderNodeInsert(const OrderNodePtr& node)
	{
		SafeMutexLock lock(mutex_);
		queue_.push_back(node);
	}

	OrderNodePtr dealOrderNode()
	{
		SafeMutexLock lock(mutex_);
		OrderNodePtr node(nullptr);
		if(!queue_.empty())
		{
			node = queue_.front();
			queue_.pop_front();
		}
		return node;
	}

private:
	MutexLock mutex_;
	OrderNodePtrDeque queue_;
};


}

#endif
