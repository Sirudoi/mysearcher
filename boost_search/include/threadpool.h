#ifndef INCLUDE_THREADPOOL_H
#define INCLUDE_THREADPOOL_H

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <list>


class TaskQueue {
public:
    TaskQueue() = default;
    virtual ~TaskQueue() = default;

    virtual void join_queue(std::function<void()> fn) = 0;
    virtual void shutdown() = 0;

};

class ThreadPool : public TaskQueue {
public:
    ThreadPool(const ThreadPool &tpool) = delete;
    virtual ~ThreadPool() override = default;

    static ThreadPool* getInstance();

    void join_queue(std::function<void()> fn) override;
    void shutdown() override;
    void thread_work();

private:
    ThreadPool();
    explicit ThreadPool(size_t n);

private:
    friend class Worker;

    static ThreadPool* ins_;
    static std::mutex mtx_;

    std::vector<std::thread> thread_pool_;
    std::list<std::function<void()>> jobs_;

    bool shutdown_;
    std::condition_variable cond_;
};

#endif // INCLUDE_THREADPOOL_H