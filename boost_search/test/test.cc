#include <string>
#include <functional>
#include "../muduo/muduo_include/TcpServer.h"

class TestServer {
public:
    TestServer(EventLoop *loop, const InetAddress &addr, const std::string &name)
        : server_(loop, addr, name)
        , loop_(loop) {
        // 注册回调函数
        server_.setConnectionCallback(
            std::bind(&TestServer::onConnection, this, std::placeholders::_1));
        
        server_.setMessageCallback(
            std::bind(&TestServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        // 设置合适的subloop线程数量
        server_.setThreadNum(3);
    }

    void start() {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr &conn) {
        if (conn->connected()) {
            std::cout << "New TcpConnection in:" << conn->peerAddress().toIpPort().c_str() << std::endl;
        }
        else {
            std::cout << "A Connection out:" << conn->peerAddress().toIpPort().c_str() << std::endl;
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time) {
        std::string msg = buf->retrieveAllAsString();
        std::cout << "Read from buffer"
                  << msg << std::endl;
        conn->send(msg);
        // conn->shutdown();   // 关闭写端 底层响应EPOLLHUP => 执行closeCallback_
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8080);
    TestServer server(&loop, addr, "TestServer");
    /**
     * 1.初始化TcpServer底层线程池的InitCallback, 这里没有传入
     * 2.创建若干个Eventloop, 设置3个就是三个
     * 3.执行Acceptor类内的listen方法, 该方法会创建套接字, 监听绑定8002端口
     * 4.将listen套接字的读时间注册到epoll中, 注册到epoll分为下面几步
     *  4.1 调用对应eventloop的update方法
     *  4.2 调用对应eventloop里面的epollpoller的update方法
     *  4.3 调用epoll_ctl添加到epoll中
     * 5.设置listen套接字绑定的Channel中的readCallback为Acceptor::handleRead()这个函数执行TcpServer中的newConnect方法
     * 6.设置connectCallback和messageCallback(用户传入的两个回调函数), 有新TCP连接创建的时候会吧这两个回调设置到这个TCP连接的回调函数里面
    */
    server.start();

    /**
     * 循环从这个loop的epoll中获取链接
     * 1.有新连接 -> 调用listen套接字绑定的Channel的readCallback函数 -> 调用Acceptor::handleRead() -> 调用TcpServer::newConnection()
     * 2.TcpServer::newConnection()内创建一个TcpConnection类, 轮询选择一个subLoop执行这个TcpConnection的io任务
     *  2.1 subLoop会执行这个TcpConnect的TcpConnection::connectEstablished()方法
     * 4.TcpConnection初始化的时候, 会把TcpConnection::handleRead() handleWrite() handleClose() handleError()设置到此次链接的套接字中
     * 5.TcpConnection::connectEstablished()会先把这次链接的Channel和TcpConnection绑定起来, 即tie一下，然后调用用户传入的connectCallback回调
     * 6.调用subLoop里面的updateChannel方法, 把此次TCP链接的读时间注册到这个subLoop的epollpoller中
     * 
     * 后续：
     * 1.本次链接收到数据->调用TcpConnection::handleRead() -> 读到这个TcpConnection的inputBuffer中 -> 调用用户传入的messageCallback_
     * 2.messageCallback_中获取这个Tcp链接的buffer缓冲区的指针
     *  2.1 调用buffer的retrieveAllAsString方法
     *  2.2 调用retrieveAsString(readableBytes())  -> readableBytes()是可读取的数据大小
     *  2.3 调用retrieveAsString(size_t len) 返回可读取数据
     *  2.4 调用retrieve更新readIndex
     * 
     * 3.调用TcpConnect的send方法写数据
     *  3.1 当前线程的subLoop->调用sendInLoop() 否则调用runInLoop(std::bind(&TcpConnection::sendInLoop())
     *  3.2 调用TcpConnect的sendInLoop方法, 把数据写出去
    */
    loop.loop();

    return 0;
}