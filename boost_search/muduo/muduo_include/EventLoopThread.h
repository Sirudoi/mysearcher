#ifndef INCLUDE_MUDUO_EVENTLOOPTHREAD_H
#define INCLUDE_MUDUO_EVENTLOOPTHREAD_H

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

#include "noncopyable.h"
#include "Thread.h"

class EventLoop;

class EventLoopThread : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop *startLoop();

private:
    void threadFunc();

    EventLoop *loop_;                   // 该eventloop线程拥有的eventloop事件循环
    bool exiting_;                      // 标志该eventloop线程已经退出
    Thread thread_;                     // 具体的线程类
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};

#endif // INCLUDE_MUDUO_EVENTLOOPTHREAD_H