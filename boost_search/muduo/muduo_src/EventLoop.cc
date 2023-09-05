#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"

// 当前线程是否创建eventloop标志, 防止一个线程创建多个eventloop
__thread EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000; // epoll等待的超时事件

/**
 * @brief 创建一个eventfd, 把该fd注册到eventloop的epoll中, 这样向这个eventfd写一个数据, 就可以唤醒eventloop
 * @return int eventfd
 */
int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        // TODO:log
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_)) {
        
    if (t_loopInThisThread) {
        // TODO：log
    }
    else {
        t_loopInThisThread = this;
    }
    

    /**
     * 将eventfd的读事件注册到该eventloop的epoll中, 此时就可以通过向eventfd执行写操作,
     * 来唤醒eventloop所在的线程, 以实现两个线程之间的通信的操作
     */

    // 设置eventfd读事件触发的回调函数
    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this));
    
    // 将eventfd的读事件注册到epoll中
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll(); // 将eventfd从eventloop的epoll中移除
    wakeupChannel_->remove();     // 将eventfd从eventloop中移除
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

/**
 * @brief 读取eventfd中的数据
 */
void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        // TODO:log
    }
}

/**
 * @brief 向eventfd写数据, 由于向epoll注册了eventfd的读事件就绪, 因此采用向eventfd写数据的方式
 *        就可以实现唤醒所属eventloop线程的操作
 */
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        // TODO:log
    }
}

/**
 * @brief           向该循环事件的epoll更新Channel事件
 * @param channel   需要更新的Channel
 */
void EventLoop::updateChannel(Channel *channel) {
    poller_->updateChannel(channel);
}

/**
 * @brief           向该循环事件的epoll删除Channel事件
 * @param channel   需要删除的Channel
 */
void EventLoop::removeChannel(Channel *channel) {
    poller_->removeChannel(channel);
}

/**
 * @brief           判断某个Channel是否在当前事件循环的epoll中
 * @param channel   需要查看的Channel
 */
bool EventLoop::hasChannel(Channel *channel) {
    return poller_->hasChannel(channel);
}

/**
 * @brief 执行用户注册的上层回调
 */
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    // 标识正在执行上层回调
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 交换见笑了锁的范围, 同时避免上层回调可能会申请锁, 这样会导致思索的情况, 提升效率
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors) {
        functor(); // 执行当前loop需要执行的回调操作
    }

    // 重置标识
    callingPendingFunctors_ = false;
}

/**
 * @brief 开启事件循环
 */
void EventLoop::loop() {
    looping_ = true;
    quit_ = false;

    // TODO:log

    while (!quit_) {
        activeChannels_.clear();
        pollRetureTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_) {
            // 执行有事件就绪的Channel上注册好的回调函数
            channel->handleEvent(pollRetureTime_);
        }
        /**
         * 执行当前EventLoop事件循环需要处理的回调操作 对于线程数 >=2 的情况 IO线程 mainloop(mainReactor) 主要工作：
         * accept接收连接 => 将accept返回的connfd打包为Channel => TcpServer::newConnection通过轮询将TcpConnection对象分配给subloop处理
         *
         * mainloop调用queueInLoop将回调加入subloop（该回调需要subloop执行 但subloop还在poller_->poll处阻塞） queueInLoop通过wakeup将subloop唤醒
         **/
        doPendingFunctors();
    }
    looping_ = false;
}

/**
 * @brief 关闭事件循环
 * 1. 如果关闭eventloop的是创造该eventloop的线程, 则直接关闭
 * 2. 如果关闭eventloop的不是创造该eventloop的线程, 则需要唤醒该eventloop所属线程, 让其所属线程关闭eventloop
 *    即让其所属线程再执行一遍epoll_wait的循环, 保证所有任务都处理完毕, 在关闭
 */
void EventLoop::quit() {
    quit_ = true;

    if (!isInLoopThread()) {
        wakeup();
    }
}

/**
 * @brief    让所属线程执行回调函数  
 * @param cb 上层回调函数
 */
void EventLoop::runInLoop(Functor cb) {
    // 该eventloop属于当前线程, 直接执行回调
    if (isInLoopThread()) {
        cb();
    }
    // 该eventloop不属于当前线程, 唤醒该eventloop所在线程, 让其所在线程执行回调
    else {
        queueInLoop(cb);
    }
}

/**
 * @brief    把上层回调放入到eventloop中
 * @param cb 上层回调
 */
void EventLoop::queueInLoop(Functor cb) {
    // 将该回调插入到该eventloop的回调执行队列中
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 如果当前eventloop不属于当前线程 || 当前eventloop正处在 "执行上层回调这一过程", 则立刻唤醒该eventloop
    // 此时当前eventloop执行完doPendingFunctors之后, epoll_wait会马上返回, eventloop会再执行doPendingFunctors
    // 否则该eventloop会等待十秒, 或者有新事件就绪后才会执行doPendingFunctors, 而不是马上执行上层回调
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}
