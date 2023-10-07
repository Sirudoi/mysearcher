#include <stdlib.h>

#include "../muduo_include/Poller.h"
#include "../muduo_include/EPollPoller.h"

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