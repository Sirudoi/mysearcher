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

    server.start();
    loop.loop();

    return 0;
}