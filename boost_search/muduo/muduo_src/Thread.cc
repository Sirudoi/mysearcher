#include "../muduo_include/Thread.h"
#include "../muduo_include/CurrentThread.h"
#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name) {
    setDefaultName();
}

Thread::~Thread() {
    // 如果该线程正在运行, 则detach分离, 其执行完回调后会由操作系统回收
    if (started_ && !joined_) {
        thread_->detach();
    }
}

/**
 * @brief 开启线程回调
 */
void Thread::start() {
    started_ = true;
    sem_t sem;
    // false指不设置进程间共享
    sem_init(&sem, false, 0);  

    // 开启线程, 执行设置的回调函数
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]() {
        tid_ = CurrentThread::tid();
        // 获取线程id之后, 唤醒在sem条件变量下等待的线程, 保证该线程执行的时候拥有线程id
        sem_post(&sem);
        // 运行回调
        func_(); 
    }));

    // 在sem下等待, 该线程获取tid之后会被唤醒
    sem_wait(&sem);
}

/**
 * @brief 设置线程默认名字, 同时对线程计数++, 默认为Thread1, Thread2
 */
void Thread::setDefaultName() {
    int num = ++numCreated_;
    if (name_.empty()) {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}

/**
 * @brief 等待该线程执行完毕, 由父进程回收资源
 */
void Thread::join() {
    joined_ = true;
    thread_->join();
}
