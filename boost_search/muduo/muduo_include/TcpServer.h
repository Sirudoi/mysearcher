#ifndef INCLUDE_MUDUO_TCPSERVER_H
#define INCLUDE_MUDUO_TCPSERVER_H

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

class TcpServer {
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum Option {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
              const InetAddress &listenAddr,
              const std::string &nameArg,
              Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    // 设置底层subloop的个数
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();

    const std::string& ipPort() const { return ipPort_; }
    const std::string& name() const { return name_; }
    EventLoop* getLoop() const { return loop_; }

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_;                                   // baseloop 用户自定义的loop

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;                // 运行在mainloop的accepter, 作用就是监听新连接

    std::shared_ptr<EventLoopThreadPool> threadPool_;   // one loop per thread

    ConnectionCallback connectionCallback_;             // 有新连接时的回调
    MessageCallback messageCallback_;                   // 有读写事件发生时的回调
    WriteCompleteCallback writeCompleteCallback_;       // 消息发送完成后的回调
    ThreadInitCallback threadInitCallback_;             // loop线程初始化的回调

    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_;                         // 保存所有的连接
};

#endif // INCLUDE_MUDUO_TCPSERVER_H