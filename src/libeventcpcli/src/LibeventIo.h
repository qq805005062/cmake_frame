#ifndef __LIBEVENT_TCPCLI_IO_H__
#define __LIBEVENT_TCPCLI_IO_H__
//#include "LibeventIo.h"

#include <event2/event.h>
#include <event2/event_struct.h>

#include "OrderInfo.h"
#include "TcpClient.h"

namespace LIBEVENT_TCP_CLI
{

class LibeventIo
{
public:

	LibeventIo();

	~LibeventIo();

	int libeventIoReady();

	int libeventIoExit();

	int libeventIoOrder(const OrderNodePtr& node); 
	
////////////////////////////////////////////////////////////////////////////////////
	void handleRead();
private:

	int libeventIoWakeup();
	
	int wakeupFd;

	OrderNodeDeque orderDeque_;
	struct event_base *evbase;
	struct event wake_event;
};

typedef std::shared_ptr<LibeventIo> LibeventIoPtr;

}

#endif
