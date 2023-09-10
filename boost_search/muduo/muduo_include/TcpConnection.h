#ifndef INCLUDE_MUDUO_TCPCONNECTION_H
#define INCLUDE_MUDUO_TCPCONNECTION_H

#include <memory>
#include <string>
#include <atomic>

#include "Callback.h"
#include "noncopyable.h"
#include "InetAddress.h"
#include "Buffer.h"

class Channel;
class EventLoop;
class Socket;

// std::enable_shared_from_this<TcpConnection>继承之后, 该类内部可以通过shared_from_this()来获取一个该类的智能指针
class TcpConnect : public noncopyable, public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop *loop,
                  const std::string &nameArg,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }

    // 判断是否建立连接
    bool connected() const { return state_ == kConnected; }
    

    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();

    // 发送数据
    void send(const std::string &buf);
    // 关闭连接 -- 关闭服务器写端的操作, 半关闭状态
    void shutdown();

    // 设置回调
    void setConnectionCallback(const ConnectionCallback &cb)
    { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb)
    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    { writeCompleteCallback_ = cb; }
    void setCloseCallback(const CloseCallback &cb)
    { closeCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

private:
    enum StateE {
        kDisconnected, // 已经断开连接
        kConnecting,   // 正在连接
        kConnected,    // 已连接
        kDisconnecting // 正在断开连接
    };

    void setState(StateE state) { state_ = state; }

    // 注册到每个fd绑定的Channel内部的具体回调
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *data, size_t len);
    void shutdownInLoop();

    EventLoop *loop_;                       // 如果线程池数量为0, 则该指针指向baseLoop
    const std::string name_;                // 链接名字
    std::atomic_int state_;                 // 当前TCP连接的状态
    bool reading_;                          // 

    std::unique_ptr<Socket> socket_;        // 该链接的fd绑定的Socket
    std::unique_ptr<Channel> channel_;      // 该链接的fd绑定的channel

    const InetAddress localAddr_;           // 本机网络信息
    const InetAddress peerAddr_;            // 对端网络信息

    ConnectionCallback connectionCallback_;         // 有新连接时的回调
    MessageCallback messageCallback_;               // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;   // 消息发送完成以后的回调
    HighWaterMarkCallback highWaterMarkCallback_;   // 控制收发速度
    CloseCallback closeCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_;    // 接收缓冲区
    Buffer outputBuffer_;   // 发送缓冲区

    // inputBuffer_.readFd()   --> 从fd中读取内核缓冲区的数据到inputBuffer_
    // outputBuffer_.writeFd() --> 从outputBuffer_写入数据到fd的内核缓冲区
};

#endif // INCLUDE_MUDUO_TCPCONNECTION_H