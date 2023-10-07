#ifndef INCLUDE_MUDUO_ACCEPTOR_H
#define INCLUDE_MUDUO_ACCEPTOR_H

#include <functional>

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
// #include "../../include/log.hpp"

class EventLoop;
class InetAddress;

class Acceptor : public noncopyable {
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb) { NewConnectionCallback_ = cb; }

    bool listenning() const { return listenning_; }
    void listen();

private:
    void handleRead();

    // Acceptor使用的eventloop, 就是用户定义的事件循环, 即baseLoop
    // 该eventloop只负责链接的建立, 别的eventloop则负责事件的读写
    // 如果用户定义的eventloop只有baseLoop, 则该eventloop负责所有操作
    EventLoop *loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback NewConnectionCallback_;
    bool listenning_;
};

#endif //INCLUDE_MUDUO_ACCEPTOR_H