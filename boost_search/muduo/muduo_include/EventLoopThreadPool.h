#ifndef INCLUDE_MUDUO_EVENTLOOPTHREADPOOL_H
#define INCLUDE_MUDUO_EVENTLOOPTHREADPOOL_H

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : public noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    // 默认以轮询方式, 分配Channel
    EventLoop *getNextLoop();

    std::vector<EventLoop *> getAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }

private:
    EventLoop *baseLoop_; // 用户使用muduo创建的loop 如果线程数为1 那直接使用用户创建的loop 否则创建多EventLoop
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;                                                  // 轮询的下标
    std::vector<std::unique_ptr<EventLoopThread>> threads_;     // 保存所有的线程
    std::vector<EventLoop *> loops_;                            // 保存所有的事件循环
};

#endif // INCLUDE_MUDUO_EVENTLOOPTHREADPOOL_H