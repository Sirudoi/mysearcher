#include <stdlib.h>

#include "Poller.h"
#include "EPollPoller.h"

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        // TODO:这里不实现epoll
        return nullptr;
    }
    else
    {
        return new EPollPoller(loop); // 生成epoll的实例
    }
}