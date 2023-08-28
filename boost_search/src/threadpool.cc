#include "../include/threadpool.h"
#include "./log.hpp"

using namespace ns_log;

ThreadPool* ThreadPool::ins_ = nullptr;
std::mutex ThreadPool::mtx_;

ThreadPool::ThreadPool() {

}

ThreadPool::ThreadPool(size_t n) {
    // 创建线程池
    for (int i = 0; i < n; ++i) {
        thread_pool_.emplace_back(std::thread(&ThreadPool::thread_work, this));
    }
}

/**
 * @brief 获取线程池单例
 * @return ThreadPool* 
 */
ThreadPool* ThreadPool::getInstance() {
    if (nullptr == ins_) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (nullptr == ins_) {
            ins_ = new ThreadPool();
        }
    }

    return ins_;
}

/**
 * @brief 任务入队
 * @param fn 线程池需要执行的函数
 */
void ThreadPool::join_queue(std::function<void()> fn) {
    std::unique_lock<std::mutex> lock(mtx_);
    jobs_.push_back(std::move(fn));
    cond_.notify_one();
}

/**
 * @brief 关闭线程池
 */
void ThreadPool::shutdown() { 
    {
        std::unique_lock<std::mutex> lock(mtx_);
        shutdown_ = true;
    }

    cond_.notify_all();

    // 等待所有线程执行完毕
    for (auto &t : thread_pool_) {
        t.join();
    }
}

/**
 * @brief 线程逻辑
 */
void ThreadPool::thread_work() {
    for (;;) {
        std::function<void()> fn;

        {
            std::unique_lock<std::mutex> lock(mtx_);
            // 任务队列不为空, 或线程池停止, 表示有事件触发
            cond_.wait(lock, [this] { return shutdown_ || !jobs_.empty(); });

            if (shutdown_ && jobs_.empty()) {
                break;
            }

            // 任务队列头获取任务
            fn = jobs_.front();
            jobs_.pop_front();
        }

        if (false == static_cast<bool>(fn)) {
            LOG(WARN) << "任务函数为空, 请检查" << std::endl;
            break;
        }
        fn();
    }
}