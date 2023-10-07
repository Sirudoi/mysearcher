#ifndef INCLUDE_MUDUO_CHANNEL_H
#define INCLUDE_MUDUO_CHANNEL_H

#include <functional>
#include <memory>

#include "./noncopyable.h"
#include "./EventLoop.h"

class Channel : public noncopyable {
public:
    using EventCallBack = std::function<void()>;
    using ReadEventCallBack = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    virtual ~Channel();

    // 调用具体事件就绪的方法
    void handleEvent(Timestamp receiveTime);

    // 设置对应回调事件
    void setReadCallback(ReadEventCallBack callback) { readCallBack_ = std::move(callback); }
    void setWriteCallback(EventCallBack callback) { writeCallBack_ = std::move(callback); }
    void setCloseCallback(EventCallBack callback) { closeCallBack_ = std::move(callback); }
    void setErrorCallback(EventCallBack callback) { errorCallBack_ = std::move(callback); }

    // TODO
    void tie(const std::shared_ptr<void> &);

    // 获取文件描述符事件
    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }

    // 设置fd事件
    // Channel->update() --> 其所属eventloop->update() --> eventloop内部的poller->update() --> epoll_ctl() 
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    // TODO
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread
    EventLoop *ownerLoop() { return loop_; }
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    EventLoop *loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;

    // TODO
    std::weak_ptr<void> tie_;
    bool tied_;

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    ReadEventCallBack readCallBack_;     // 读事件回调
    EventCallBack writeCallBack_;       // 写事件回调
    EventCallBack closeCallBack_;       // 关闭事件回调
    EventCallBack errorCallBack_;       // 错误事件回调
};

#endif // INCLUDE_MUDUO_CHANNEL_H