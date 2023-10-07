#ifndef INCLUDE_MUDUO_POLLER_H
#define INCLUDE_MUDUO_POLLER_H

#include <vector>
#include <unordered_map>

#include "./noncopyable.h"
#include "./Timestamp.h"

class Channel;
class EventLoop;

class Poller {
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    bool hasChannel(Channel *channel) const;

    // 通过该接口能获取IO复用具体是使用Poll还是Epoll
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    // { sockfd, sockfd绑定的Channel对象 }
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;

private:
    // 定义Poller使用epoll还是poll
    EventLoop *ownerLoop_;
};

#endif // INCLUDE_MUDUO_POLLER_H