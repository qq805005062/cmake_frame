#ifndef __COMMON_LIBEVENT_IO_H__
#define __COMMON_LIBEVENT_IO_H__

#include <event2/event.h>
#include <event2/event_struct.h>

#include "OrderInfo.h"

namespace common
{
class LibeventIo
{
public:

    LibeventIo();

    ~LibeventIo();

    int libeventIoReady();

    void libeventIoDispatch();

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

}//end namespace common
#endif //end __COMMON_LIBEVENT_IO_H__
