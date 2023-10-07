#ifndef INCLUDE_MUDUO_SOCKET_H
#define INCLUDE_MUDUO_SOCKET_H

#include "noncopyable.h"

class InetAddress;

// 封装socket fd
class Socket : public noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd) {
    }
    ~Socket();

    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};

#endif // INCLUDE_MUDUO_SOCKET_H