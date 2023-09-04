#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "EPollPoller.h"
#include "Channel.h"

const int kNew = -1;    // 某个channel还没添加至Poller
const int kAdded = 1;   // 某个channel已经添加至Poller
const int kDeleted = 2; // 某个channel已经从Poller删除

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC)) /* 调用exec后关闭epollfd, 创建子进程自动关闭epollfd */
    , events_(kInitEventListSize) /* vector<Channel*>(16) */ {
    if (epollfd_ < 0) {
        // TODO: 打印log
    }
}

EPollPoller::~EPollPoller() {
    ::close(epollfd_);
}

/**
 * @brief                   epoll逻辑, 等待事件, 将事件添加到epoll数组, 返回每个channel的就绪事件, 即revents
 * @param timeoutMs         等待超时时间
 * @param activeChannels    输出型参数, 返回有事件就绪的channel列表
 * @return Timestamp        当前时间戳
 */
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
    // &*events_.begin() -> epoll数组第一个元素的地址 static_cast<int>(events_.size()) -> epoll数组的大小, 
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    // 有事件就绪
    if (numEvents > 0) {
        // 获取有事件就绪的Channel
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    }
    // 等待超时
    else if (numEvents == 0) {
        // TODO:打印log
    }
    // 其他错误
    else {
        // TODO:打印log

        // 系统中断
        if (saveErrno != EINTR) {
            errno = saveErrno;
        }
    }

    return now;
}

/**
 * @brief                   返回此次等待后有事件就绪的Channel
 * @param numEvents         就绪的Channel数量
 * @param activeChannels    就绪的Channel数组
 */
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) {
    for (int i = 0; i < numEvents; ++i) {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

/**
 * @brief           更新Channel内部的事件(base method call)
 * @param operation 更新选项 EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL
 * @param channel   更新的Channel
 */
void EPollPoller::update(int operation, Channel *channel) {
    epoll_event event;
    ::memset(&event, 0, sizeof(event));

    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            // TODO:print log
        }
        else {
            // TODO:print log
        }
    }
}

/**
 * @brief           移除某个Channel
 * @param channel   需要移除的Channel
 */
void EPollPoller::removeChannel(Channel *channel) {
    int fd = channel->fd();
    // 从 {fd, Channel*}map 中移除
    channels_.erase(fd);

    // TODO:LOG

    // 从epoll中移除
    int index = channel->index();
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    // 更改状态
    channel->set_index(kNew);
}

/**
 * @brief           更改Channel的状态 
 * @param channel   需要更改的Channel
 */
void EPollPoller::updateChannel(Channel *channel) {
    const int index = channel->index();

    // TODO:log

    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    // kAdded
    else {
        // TODO
        // delete
        if () {

        }
        // modify
        else {

        }
    }
}