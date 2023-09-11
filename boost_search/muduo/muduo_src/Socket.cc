#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

Socket::~Socket() {
    ::close(sockfd_);
}

/**
 * @brief           绑定ip和端口号
 * @param localaddr sockaddr_in结构体
 */
void Socket::bindAddress(const InetAddress &localaddr) {
    if (0 != ::bind(sockfd_, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in))) {
        // TODO:lOG
    }
}

/**
 * @brief 监听套接字
 */
void Socket::listen() {
    if (0 != ::listen(sockfd_, 1024)) {
        // TODO:log
    }
}

/**
 * @brief            从listen套接字获取链接
 * @param peeraddr   对端peer的信息   
 */
int Socket::accept(InetAddress *peeraddr) {
    /**
     * 1. accept函数的参数不合法
     * 2. 对返回的connfd没有设置非阻塞
     * Reactor模型 one loop per thread
     * poller + non-blocking IO
     **/
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    ::memset(&addr, 0, sizeof(addr));
    // fixed : int connfd = ::accept(sockfd_, (sockaddr *)&addr, &len);
    // accept获取之后需要手动setsockopt, accept4可以获取同时设置
    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0) {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

/**
 * @brief 关闭写端
 *   #include <sys/socket.h>
 *   int shutdown(int sockfd, int how);
 *   SHUT_RD SHUT_WR SHUT_RDWR --> 分别关闭读 写 读写
 */
void Socket::shutdownWrite() {
    if (::shutdown(sockfd_, SHUT_WR) < 0) {
        LOG_ERROR("shutdownWrite error");
    }
}

/**
 * @brief 用于禁用或启用 Nagle 算法。Nagle 算法可通过将小数据块缓冲在一起来提高网络传输效率，但会增加延迟
 * @param on 
 */
void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

/**
 * @brief 设置允许端口复用, 即在time_wait状态下绑定端口
 * @param on 
 */
void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

/**
 * @brief 设置允许多个套接字监听一个端口
 * @param on 
 */
void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

/**
 * @brief 设置长时间空间后发送心跳检测
 * @param on 
 */
void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}