
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/cdefs.h>

#include <unistd.h>

#include "Incommon.h"
#include "LibeventIo.h"

#pragma GCC diagnostic ignored "-Wold-style-cast"
namespace common
{
static int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        PERROR("Failed in eventfd");
    }
    return evtfd;
}

static void wakeUpFdcb(int sockFd, short eventType, void *arg)
{
    LibeventIo *p = (LibeventIo *)arg;
    p->handleRead();
}

LibeventIo::LibeventIo()
    :wakeupFd(createEventfd())
    ,orderDeque_()
    ,evbase(nullptr)
    ,wake_event()
{
    PDEBUG("LibeventIo init");
}

LibeventIo::~LibeventIo()
{
    PERROR("~LibeventIo exit");
}

int LibeventIo::libeventIoReady()
{
    PDEBUG("libeventIoReady in");
    if(wakeupFd < 0)
    {
        PERROR("libeventIoReady wakeupFd_ error %d", wakeupFd);
        return -1;
    }

    evbase = event_base_new();
    if(evbase == nullptr)
    {
        PERROR("libeventIoReady event_base_new new error");
        return -1;
    }

    event_assign(&wake_event, evbase, wakeupFd, EV_READ|EV_PERSIST, wakeUpFdcb, this);
    event_add(&wake_event, NULL);

    event_base_dispatch(evbase);
    PERROR("event_base_dispatch return %p", evbase);

    event_del(&wake_event);
    close(wakeupFd);
    event_base_free(evbase);
    evbase = nullptr;
    return 0;
}

int LibeventIo::libeventIoOrder(const OrderNodePtr& node)
{
    orderDeque_.orderNodeInsert(node);
    libeventIoWakeup();
    return 0;
}

int LibeventIo::libeventIoWakeup()
{
    uint64_t one = 2;
    ssize_t n = write(wakeupFd, &one, sizeof one);
    //INFO("wakeup n one %ld %ld %p", n, one, evbase);
    if (n != sizeof one)
    {
        PERROR("EventLoop::wakeup() writes %ld bytes instead of 8", n);
    }
    return 0;
}

int LibeventIo::libeventIoExit()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd, &one, sizeof one);
    PDEBUG("LibeventIo n one %ld %ld %p", n, one, evbase);
    if (n != sizeof one)
    {
        PERROR("EventLoop::wakeup() writes %ld bytes instead of 8", n);
    }
    return 0;
}

void LibeventIo::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd, &one, sizeof one);
    if (n != sizeof one)
    {
        PERROR("LibeventIo::handleRead() reads %ld bytes instead of 8", n);
        return;
    }
    PDEBUG("handleRead n one %ld %ld :: %p", n, one, evbase);

    while(1)
    {
        OrderNodePtr node = orderDeque_.dealOrderNode();
        if(node)
        {
            ;//TODO
        }else{
            break;
        }
    }
    uint64_t num = one % 2;
    if(num)
    {
        event_base_loopbreak(evbase);
    }
}

}

