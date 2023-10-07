#ifndef INCLUDE_MUDUO_EVENTLOOP_H
#define INCLUDE_MUDUO_EVENTLOOP_H

#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include <functional>


#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

class EventLoop : public noncopyable {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    // epoll_wait返回的时间
    Timestamp pollReturnTime() const { return pollRetureTime_; }

    // 在当前loop中执行
    void runInLoop(Functor cb);
    // 把上层注册的回调函数放入队列, 唤醒该回调对应loop所属线程执行回调
    void queueInLoop(Functor cb);

    // 通过eventfd唤醒loop所在的线程
    void wakeup();

    // 调用epollPoller类的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断某一事件的所在的eventloop, 是否在自己的线程里
    // CurrentThread::tid()用于获取当前线程的id
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
private:
    void handleRead();
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;                  // 标识该事件循环正在运行
    std::atomic_bool quit_;                     // 标识该事件循环已经退出

    Timestamp pollRetureTime_;                  // epoll_wait返回的时间戳
    std::unique_ptr<Poller> poller_;            // 该eventloop的epoll对象

    const pid_t threadId_;                      // 记录创建该EventLoop的线程id

    int wakeupFd_;                              // eventfd的文件描述符
    std::unique_ptr<Channel> wakeupChannel_;    // eventfd绑定的Channel

    ChannelList activeChannels_;                // 获取epoll返回的所有有事件就绪的Channel

    std::atomic_bool callingPendingFunctors_;   // 标识当前事件循环是否执行上层回调
    std::vector<Functor> pendingFunctors_;      // 存储所有上层回调的数组
    std::mutex mutex_;
};

#endif // INCLUDE_MUDUO_EVENTLOOP_H