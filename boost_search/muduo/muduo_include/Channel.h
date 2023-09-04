#ifndef INCLUDE_MUDUO_CHANNEL_H
#define INCLUDE_MUDUO_CHANNEL_H

#include <functional>
#include <memory>

#include "./noncopyable.h"

class Channel : public noncopyable {
public:
    using EventCallBack = std::functional<void()>;
    using ReadEventCalBack = std::functional<void(Timestamp)>;

    Channel();
    virtual ~Channel();

    // 设置对应回调事件
    void setReadCallback(ReadEventCalBack callback) { readCallBack_ = std::move(callback); }
    void setWriteCallback(EventCallBack callback) { writeCallBack_ = std::move(callback); }
    void setCloseCallback(EventCallBack callback) { closeCallBack_ = std::move(callback); }
    void setErrorCallback(EventCallBack callback) { errorCallBack_ = std::move(callback); }

    // 获取文件描述符事件
    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }


private:

    EventLoop *loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;

    ReadEventCalBack readCallBack_;     // 读事件回调
    EventCallBack writeCallBack_;       // 写事件回调
    EventCallBack closeCallBack_;       // 关闭事件回调
    EventCallBack errorCallBack_;       // 错误事件回调
};

#endif // INCLUDE_MUDUO_CHANNEL_H