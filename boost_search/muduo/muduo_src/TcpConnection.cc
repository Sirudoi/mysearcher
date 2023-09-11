#include <functional>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

#include "../muduo_include/TcpConnection.h"
#include "../muduo_include/Socket.h"
#include "../muduo_include/Channel.h"
#include "../muduo_include/EventLoop.h"

/**
 * @brief 检查loop是否为空
 */
static EventLoop *CheckLoopNotNull(EventLoop *loop) {
    if (loop == nullptr) {
        // LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
        std::cout << "mainLoop is null" << std::endl;   
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(nameArg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64 * 1024 * 1024) {
    // 给当前TCP连接的Channel设置回调, epoll返回事件后会触发回调
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    // LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

/**
 * @brief       发送buf缓冲区的数据
 * @param buf   待发送的缓冲区
 */
void TcpConnection::send(const std::string &buf) {
    if (state_ == kConnected) {
        // 如果当前事件循环属于这个线程
        if (loop_->isInLoopThread()) {
            // 在当前事件循环执行
            sendInLoop(buf.c_str(), buf.size());
        }
        else {
            // 否则执行runInLoop -> runInLoop内部会检查当前eventloop是否属于当前thread
            // 属于会执行回调方法, 不属于则会直接唤醒当前eventloop所在线程, 让其执行回调方法
            loop_->runInLoop(
                std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

/**
 * @brief       在当前事件循环发送buffer缓冲区的数据
 * @param data  buf起始地址
 * @param len   字节数
 */
void TcpConnection::sendInLoop(const void *data, size_t len) {
    ssize_t nwrote = 0;         // 实际写入字节数
    size_t remaining = len;     // 剩余还需写入的字节数  
    bool faultError = false;    // 是否发生错误

    // 连接已关闭, 不用发送了
    if (state_ == kDisconnected) {
        // LOG_ERROR("disconnected, give up writing");
    }

    // 该Channel关心写事件, 且读缓冲区没有数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            // 该次发送将buf的数据全部发送出去了
            if (remaining == 0 && writeCompleteCallback_) {
                // 调用写事件回调
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        // nwrote < 0
        else {
            nwrote = 0;
            // EWOULDBLOCK表示非阻塞情况下没有数据后的正常返回 等同于EAGAIN
            if (errno != EWOULDBLOCK) {
                // LOG_ERROR("TcpConnection::sendInLoop");
                // SIGPIPE RESET
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }
    /**
     * remaining > 0, 说明当前这一次write并没有把数据全部发送出去, 还需要下面的操作
     * 1. 需要把剩余的数据保存到写缓冲区中, 即保存到outbuffer_中
     * 2. 为该Channel注册epollout事件, epoll_wait发现内核TCP缓冲区有空间会返回写事件就绪
     * 
     * Channel注册的writeCallback_回调 -> 绑定的就是TcpConnection的handlewrite
     * 以此保证把发送缓冲区outputBuffer_的内容全部发送完成
     **/
    if (!faultError && remaining > 0) {
        // 当前缓冲区待发送的数据长度
        size_t oldLen = outputBuffer_.readableBytes();
        // 之前未发送 + 本次还需发送 >= 水位线, 且设置了水位线回调, 则将水位线回调放入到eventloop的事件队列中 
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_) {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }

        // 将本次未写入完的数据追加到缓冲区
        outputBuffer_.append((char *)data + nwrote, remaining);

        // 如果该Channel没有将写事件注册到epoll, 则需要注册
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

/**
 * @brief 关闭当前连接, 关闭tcp连接后服务器会先关闭写端口
 */
void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

/**
 * @brief 关闭该fd的写端
 */
void TcpConnection::shutdownInLoop() {
    // 当前fd是否还关心写事件 -> outbuffer_缓冲区是否写完
    if (!channel_->isWriting()) {
        // 关闭该fd的写端
        socket_->shutdownWrite();
    }
}

/**
 * @brief 连接建立
 */
void TcpConnection::connectEstablished() {
    setState(kConnected);
    channel_->tie(shared_from_this());
    // 注册该fd的读事件EPOLLIN
    channel_->enableReading();

    // 有新连接建立, 调用连接建立的回调
    connectionCallback_(shared_from_this());
}

/**
 * @brief 销毁连接
 */
void TcpConnection::connectDestroyed() {
    if (state_ == kConnected) {
        setState(kDisconnected);
        // 将该Channel对应的fd关心的事件从poller中移除
        channel_->disableAll();

        // 调用连接建立的回调函数
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // 把channel从poller中删除掉
}

/**
 * @brief 读事件触发的回调函数, poller -> 执行channel的回调 -> 当前TcpConnection的回调
 * @param receiveTime 
 */
void TcpConnection::handleRead(Timestamp receiveTime) {
    int savedErrno = 0;
    // 将数据从该fd读取到本次tcp连接的缓冲区
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    // n > 0说明读取成功
    if (n > 0) {
        // 调用用户注册的读事件发生回调, 处理上层逻辑
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    // 客户端断开
    else if (n == 0) {
        handleClose();
    }
    // 出错了
    else {
        errno = savedErrno;
        // LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

/**
 * @brief 写事件触发的回调函数, poller -> 执行channel的回调 -> 当前TcpConnection的回调
 */
void TcpConnection::handleWrite() {
    if (channel_->isWriting()) {
        int savedErrno = 0;
        // 将当前TCP连接outbuffer缓冲区的数据写入到fd中
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0) {
            // 更新outbuffer_中readIndex_的位置
            // 写入缓冲区往fd中写入数据 --> outbuffer_ 中 readableBytes 改变 --> 更新readIndex_位置
            outputBuffer_.retrieve(n);

            // outputBuffer_可读取的数据长度为0, 说明没有数据需要写入
            if (outputBuffer_.readableBytes() == 0) {
                // 关闭该fd的写事件
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    // 向该eventloop加入写事件完成的回调
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting) {
                    // 在当前所属的loop中把TcpConnection删除掉
                    shutdownInLoop();
                }
            }
        }
        else {
            // LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else {
        // LOG_ERROR("TcpConnection fd=%d is down, no more writing", channel_->fd());
    }
}

/**
 * @brief 关闭事件回调
 */
void TcpConnection::handleClose() {
    // LOG_INFO("TcpConnection::handleClose fd=%d state=%d\n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 执行连接关闭的回调
    closeCallback_(connPtr);      // 执行关闭连接的回调 执行的是TcpServer::removeConnection回调方法   // must be the last line
}

/**
 * @brief 错误事件回调, 获取本次错误码
 */
void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;

    /**
     *  #include <sys/socket.h>
     *  int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
     *  查询该套接字的相关信息
     *
     *  sockfd  - fd
     *  level   - 所属协议层, 一般传入通用的SOL_SOCKET
     *  optname - 需要查询的选项名, 这里指error错误码
     *  optval  - 返回值的缓冲区
     *  optlen  - 输入输出参数, 指定缓冲区大小
     */

    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
    }
    else {
        err = optval;
    }
    // LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", name_.c_str(), err);
}