#include "../include/tcpserver.h"

namespace ns_tcpserver {

TcpServer::TcpServer() {

}

inline bool TcpServer::start_to_listen() {
    bool ret = true;
    is_running_ = true;
    while (svr_sock_ > 0) {
        auto val = epoll_wait(svr_sock_, eparr_, EPOLL_MAX_EVENTS, 1000);

        if (val == 0) {
            // TODO:do nothing
            continue;
        }

        socket_t sock = accept(svr_sock_, nullptr, nullptr);

        // TODO:判断sock是否合法
        if (sock < 0) {
            break;
        }

        ThreadPool::getInstance()->join_queue([=, this]() { /*TODO*/ });
    }

    ThreadPool::getInstance()->shutdown();
    is_running_ = false;

    return ret;
}


}