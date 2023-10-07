#ifndef INCLUDE_MUDUO_CURRENTTHREAD_H
#define INCLUDE_MUDUO_CURRENTTHREAD_H

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread {
    // __thread是编译器扩展, 表示该变量每个线程独立拥有, 并且只有该线程能访问
    // 这样子写是能将线程ID做缓存, 不用每次调用cacheTid()获取
    // cacheTid()里面是系统掉用, 系统调用很消耗时间, 将线程id缓存到t_cachedTid能提升效率
    extern __thread int t_cachedTid; 

    void cacheTid();

    inline int tid() {
        // __builtin_expect 是一种底层优化
        // __builtin_expect(expression, expected_value)表示expression表达式的预期结果是0
        // 用这种方法可以优化幸能, 告诉编译器大部分情况下t_cachedTid值为非0, 这样底层可以重排指令提高效率
        if (__builtin_expect(t_cachedTid == 0, 0)) {
            cacheTid();
        }
        return t_cachedTid;
    }
}

#endif // INCLUDE_MUDUO_CURRENTTHREAD_H