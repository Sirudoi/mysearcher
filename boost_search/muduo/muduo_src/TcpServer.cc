#include <functional>
#include <string.h>

#include "../muduo_include/TcpServer.h"
#include "../muduo_include/TcpConnection.h"

/**
 * @brief 检查该loop是否为空, 为空返回nullptr, 否则返回该loop的指针
 */
static EventLoop *CheckLoopNotNull(EventLoop *loop) {
    if (loop == nullptr) {
        // LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const std::string &nameArg,
                     Option option)
    : loop_(CheckLoopNotNull(loop))
    , ipPort_(listenAddr.toIpPort())
    , name_(nameArg)
    , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
    , threadPool_(new EventLoopThreadPool(loop, name_))
    , connectionCallback_()
    , messageCallback_()
    , nextConnId_(1)
    , started_(0) {
    // 当有新用户连接时，Acceptor类中绑定的acceptChannel_会有读事件发生，执行handleRead()调用TcpServer::newConnection回调
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() {
    for(auto &item : connections_) {
        TcpConnectionPtr conn(item.second);
        // 把原始的智能指针复位 让栈空间的TcpConnectionPtr conn指向该对象
        // 利用conn出作用域 释放每一个TcpConnect对象
        item.second.reset();
        // 销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

/**
 * @brief 设置subLoop的个数
 */
void TcpServer::setThreadNum(int numThreads) {
    threadPool_->setThreadNum(numThreads);
}

/**
 * @brief 开启服务监听
 */
void TcpServer::start() {
    // 防止一个TcpServer对象被start多次
    if (started_++ == 0) {
        // 启动底层的loop线程池
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

/**
 * @brief 有新用户连接, 其baseLoop的acceptor都会调用这个方法, 此方法内部获取连接后轮询交给subLoop处理具体业务
 */
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
    // 轮询算法 选择一个subLoop 来管理connfd对应的channel
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);

    // 该操作只在mainLoop进行, 不涉及线程安全问题
    ++nextConnId_;
    std::string connName = name_ + buf;

    // LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n",
    //          name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    
    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::memset(&local, 0, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if(::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0) {
        // LOG_ERROR("sockets::getLocalAddr");
    }

    InetAddress localAddr(local);
    TcpConnectionPtr conn(new TcpConnection(ioLoop,     // 分配给该连接的eventloop
                                            connName,   // 该连接名字
                                            sockfd,     // 该连接的fd
                                            localAddr,  // 本地地址
                                            peerAddr)); // 对端地址

    // 将本次连接保存起来
    connections_[connName] = conn;

    // 用户设置回调 -> TcpServer将用户回调设置给每一个TcpConnection
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    // 每个Channel会设置 TCPConnection的 handleRead ... 等方法


    // 设置了如何关闭连接的回调
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    ioLoop->runInLoop(
        std::bind(&TcpConnection::connectEstablished, conn));
}

/**
 * @brief 将移除某个连接的任务, 放入eventloop的事件队列
 */
void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

/**
 * @brief 移除某个事件的具体操作
 */
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    // LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
    //          name_.c_str(), conn->name().c_str());

    // 从map中删除
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();

    // 调用该subLoop的连接关闭事件放入该subLoop的事件队列
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}