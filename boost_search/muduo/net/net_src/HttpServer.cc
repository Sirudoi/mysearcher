#include "HttpServer.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

/**
 * @brief 默认的http回调, 返回404响应码
 */
void defaultHttpCallback(const HttpRequest&, HttpResponse* resp) {
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const std::string& name,
                       TcpServer::Option option):
    server_(loop, listenAddr, name, option),
    httpCallback_(defaultHttpCallback) {

    // 设置有连接建立的回调函数
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1));

    // 设置读取消息的回调函数
    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, 
                  this, 
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));
}

/**
 * @brief 开启该HttpServer内部的TcpServer
 */
void HttpServer::start() {
//   LOG_WARN << "HttpServer[" << server_.name()
//     << "] starts listening on " << server_.ipPort();
    server_.start();
}

/**
 * @brief       有http连接建立的回调函数
 * @param conn  指向该次TCP连接的指针
 */
void HttpServer::onConnection(const TcpConnectionPtr& conn) {
    // TODO:LOG
    if (conn->connected()) {
        conn->setContext(HttpContext());
    }
}

/**
 * @brief               收到http请求的回调函数  
 * @param conn          本次TCP连接的指针
 * @param buf           本次TCP连接内部的读取缓冲区
 * @param receiveTime   接受时间
 */
void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp receiveTime) {
    HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());

    // 解析请求失败, 构建响应为400的响应, 关闭本次连接
    if (!context->parseRequest(buf, receiveTime)) {
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }

    // 如果解析全部http请求成功
    if (context->gotAll()) {
        // 调用onRequest构建响应
        onRequest(conn, context->request());
        // 重置, 清空这次获取的context
        context->reset();
    }
}

/**
 * @brief       构建本次TCP连接的响应, 并把本次响应内容发送给对端
 * @param conn  本次TCP连接的指针
 * @param req   本次TCP连接的响应
 */
void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req) {
    // 获取请求头的Connection字段的内容
    const std::string& connection = req.getHeader("Connection");
    // 如果是http1.0 或者请求头的Connection字段为close, 那本次TCP连接之后断开连接
    bool close = connection == "close" ||
        (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");


    HttpResponse response(close);
    // 调用用户层回调, 设置对应的http响应
    httpCallback_(req, &response);
    Buffer buf;
    response.appendToBuffer(&buf);
    conn->send(&buf);

    // 如果close为true, 关闭本次TCP连接
    if (response.closeConnection()) {
        conn->shutdown();
    }
}
