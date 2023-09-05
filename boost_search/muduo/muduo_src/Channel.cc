#include <sys/epoll.h>

#include "../muduo_include/Channel.h"

static const int kNoneEvent = 0;
static const int kReadEvent = EPOLLIN | EPOLLPRI; // 读事件和紧急事件
static const int kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false) {

}

Channel::~Channel() {

}

/**
 * @brief 更新该Channel需要监听的事件
*/
void Channel::update() {
    loop_->updateChannel(this);
}

/**
 * @brief 移除该Channel需要监听的事件
 */
void Channel::remove() {
    loop_->removeChannel(this);
}

/**
 * @brief             执行该Channel的事件就绪回调函数
 * @param receiveTime epoll_wait返回的时间
 */
void Channel::handleEvent(Timestamp receiveTime) {
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
        // 如果提升失败了 就不做任何处理 说明Channel的TcpConnection对象已经不存在了
    }
    else {
        handleEventWithGuard(receiveTime);
    }
}

/**
 * @brief             根据revents返回的就绪事件, 调用对应的事件就绪回调函数
 * @param receiveTime 返回时间
 */
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    // TODO:LOG

    // EPOLLHUP表示连接关闭
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) {
            // 执行关闭事件的回调函数
            closeCallback_();
        }
    }
    // EPOLLERR表示发生错误
    if (revents_ & EPOLLERR) {
        if (errorCallback_) {
            //  执行错误事件的回调函数
            errorCallback_();
        }
    }
    // EPOLLIN表示读, EPOLLPRI表示紧急事件
    if (revents_ & (EPOLLIN | EPOLLPRI)) {
        if (readCallback_) {
            // 执行读事件的回调函数
            readCallback_(receiveTime);
        }
    }
    // EPOLLOUT表示写
    if (revents_ & EPOLLOUT) {
        if (writeCallback_) {
            // 执行写事件的回调函数
            writeCallback_();
        }
    }
}

// TODO
void Channel::tie(const std::shared_ptr<void> &obj) {
    tie_ = obj;
    tied_ = true;
}