#include "CurrentThread.h"

namespace CurrentThread {
    __thread int t_cachedTid = 0;

    void cacheTid() {
        if (t_cachedTid == 0) {
            // ::syscall(SYS_gettid)系统调用, 获取线程ID, 没有用std::this_thread::get_id()不是很懂
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}