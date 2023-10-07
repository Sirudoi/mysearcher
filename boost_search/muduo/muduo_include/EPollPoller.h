#ifndef INCLUDE_MUDUO_EPOLLPOLLER_H
#define INCLUDE_MUDUO_EPOLLPOLLER_H

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"
#include "Timestamp.h"


class EPollPoller : public Poller {
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    // 初始epoll文件描述符数组大小
    static const int kInitEventListSize = 16;

    // 获取有事件的Channel数组
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

    // 更新Channel的事件, 即调用epoll_ctl
    void update(int operation, Channel *channel);

    using EventList = std::vector<struct epoll_event>;

    int epollfd_;
    EventList events_;
};

#endif // INCLUDE_MUDUO_EPOLLPOLLER_H