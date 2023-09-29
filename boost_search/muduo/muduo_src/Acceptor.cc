#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#include "../muduo_include/Acceptor.h"
#include "../muduo_include/InetAddress.h"

/**
 * @brief Create a Nonblocking sockfd
 * @return int 
 */
static int createNonblocking() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        // ns_log::LOG(ns_log::WARN) << "createNonblocking failed" << std::endl;
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking())
    , acceptChannel_(loop, acceptSocket_.fd())
    , listenning_(false) {
    // 设置套接字复用
    acceptSocket_.setReuseAddr(true);
    // 设置端口复用
    acceptSocket_.setReusePort(true);
    // 绑定
    acceptSocket_.bindAddress(listenAddr);
    // TcpServer::start() => Acceptor.listen() 如果有新用户连接 要执行一个回调(accept => connfd => 打包成Channel => 唤醒subloop)
    // baseloop监听到有事件发生 => acceptChannel_(listenfd) => 执行该回调函数
    acceptChannel_.setReadCallback(
        std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
    // 把从Poller中感兴趣的事件删除掉
    acceptChannel_.disableAll();
    // 该fd绑定的Channel调用remove => 
    // 该Channel所属的eventloop调用remove => 
    // 该eventloop内部的epoll调用epoll_ctl()移除这个fd
    acceptChannel_.remove();
}

/**
 * @brief 开启监听
 */
void Acceptor::listen() {
    listenning_ = true;
    acceptSocket_.listen();             // listen
    acceptChannel_.enableReading();     // 将listen套接字的读事件注册到epoll中
}

/**
 * @brief listenfd有事件发生, 调用此回调函数, 获取链接
 */
void Acceptor::handleRead() {
    InetAddress peerAddr;
    // 从listen中accept, 并借由peerAddr获取对端信息
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0) {
        if (NewConnectionCallback_) {
            // 轮询找到subLoop 唤醒并分发当前的新客户端的Channel
            NewConnectionCallback_(connfd, peerAddr);
        }
        else {
            ::close(connfd);
        }
    }
    else {
        // TODO -- LOG
        // EMFILE  Too many open files (POSIX.1) -- 打开的文件描述符达到上限
        if (errno == EMFILE) {
            // TODO -- LOG
        }
    }
}
