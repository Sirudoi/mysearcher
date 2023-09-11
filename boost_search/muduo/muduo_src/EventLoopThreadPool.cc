#include <memory>

#include "../muduo_include/EventLoopThreadPool.h"
#include "../muduo_include/EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0) {
}

EventLoopThreadPool::~EventLoopThreadPool() {
    // 不用手动回收, 该线程池所有的事件循环都是在栈上开辟的
}

/**
 * @brief       创建多个事件循环
 * @param cb    base事件循环执行的回调
 */
void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
    started_ = true;

    for(int i = 0; i < numThreads_; ++i) {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        // 创造EventLoopTreadh对象, 传入回调
        EventLoopThread *t = new EventLoopThread(cb, buf);
        // 将该对象保存起来
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        // EventLoopTreadh.startLoop()方法 -> 底层创建一个新线程
        // 该线程创建一个eventloop事件循环, 并且会执行这个事件循环
        // 当底层线程创造eventloop事件循环后, 该函数会有父进程返回创造的eventloop
        loops_.push_back(t->startLoop());
    }

    // 只有baseLoop一个事件循环, 则让baseLoop执行上层回调
    if(numThreads_ == 0 && cb) {
        cb(baseLoop_);
    }
}

/**
 * @brief               轮询分配Channel给每个eventloop线程创建的eventloop
 * @return EventLoop*   
 */
EventLoop *EventLoopThreadPool::getNextLoop() {
    // 默认返回mainLoop, 如果有别的事件循环, 则返回别的, 否则默认返回mainLoop的事件循环
    EventLoop *loop = baseLoop_;

    // 轮询获取下一个eventloop
    if(!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        if(next_ >= loops_.size()) {
            next_ = 0;
        }
    }

    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
    // 线程池没有eventloop线程, 返回baseloop
    if(loops_.empty()) {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else {
        return loops_;
    }
}
