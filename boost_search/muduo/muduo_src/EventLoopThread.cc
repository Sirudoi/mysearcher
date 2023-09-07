#include "../muduo_include/EventLoopThread.h"
#include "../muduo_include/EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(nullptr)
    , exiting_(false)
    // 将thread执行的回调绑定到EventLoopThread::threadFunc
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , mutex_()
    , cond_()
    , callback_(cb) {
}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) {
        loop_->quit();
        // 等待其thread执行完毕
        thread_.join();
    }
}

/**
 * @brief               启动该eventloop事件循环线程的事件循环
 * @return EventLoop*   启动的事件循环
 */
EventLoop *EventLoopThread::startLoop() {
    // 启动线程执行注册的方法
    thread_.start();

    EventLoop *loop = nullptr; 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 循环等待, 防止虚假唤醒
        while(loop_ == nullptr) {
            // 等待该eventloop线程创建属于其的事件循环
            // 该线程会在这里等待, 被唤醒时还会重新执行一次while的判断, 保证不是虚假唤醒
            cond_.wait(lock);

            // 下面这种用if可能会导致虚假唤醒, 使用if线程被唤醒就直接往下执行了, while则会再次循环检测
            // if (loop_ == nullptr)
            // cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc() {
    // 创建一个独立的EventLoop对象 和上面的线程是一一对应的 级one loop per thread
    EventLoop loop;

    if (callback_) {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    
    loop.loop();    // 执行EventLoop的loop() 开启了底层的Poller的poll()
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
