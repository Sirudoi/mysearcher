#ifndef MUDUO_NET_HTTPSERVER_INCLUDE
#define MUDUO_NET_HTTPSERVER_INCLUDE

#include "../../muduo_include/TcpServer.h"
#include <functional>

class HttpRequest;
class HttpResponse;

class HttpServer : public noncopyable {
public:
    using HttpCallback = std::function<void(const HttpRequest&, HttpResponse*)>;

    HttpServer(EventLoop* loop,
               const InetAddress& listenAddr,
               const std::string& name,
               TcpServer::Option option = TcpServer::kNoReusePort);

    EventLoop* getLoop() const { return server_.getLoop(); }

    void setHttpCallback(const HttpCallback& cb) { httpCallback_ = cb; }
    void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

    void start();

private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,
                   Buffer* buf,
                   Timestamp receiveTime);
    void onRequest(const TcpConnectionPtr&, const HttpRequest&);

    TcpServer server_;          // 服务器
    HttpCallback httpCallback_; // 接受http链接的回调, 即参数为req和resp, 函数内部设置对应resp的回调
};

#endif // MUDUO_NET_HTTPSERVER_INCLUDE