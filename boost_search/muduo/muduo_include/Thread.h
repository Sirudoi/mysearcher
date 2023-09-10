#ifndef INCLUDE_MUDUO_THREAD_H
#define INCLUDE_MUDUO_THREAD_H

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

#include "noncopyable.h"

class Thread : public noncopyable {
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() { return started_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }

    static int numCreated() { return numCreated_; }

private:
    void setDefaultName();

    bool started_;                          // 标识线程是否在运行回调
    bool joined_;                           // 表示等待
    std::shared_ptr<std::thread> thread_;   // 当前的线程
    pid_t tid_;                             // 线程id
    ThreadFunc func_;                       // 该线程执行的回调函数
    std::string name_;                      // TODO 线程名字
    static std::atomic_int numCreated_;     // 创建的线程数量, static多个线程共享
};

#endif // INCLUDE_MUDUO_THREAD_H