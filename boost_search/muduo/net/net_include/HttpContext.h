#ifndef MUDUO_NET_HTTPCONTEXT_INCLUDE
#define MUDUO_NET_HTTPCONTEXT_INCLUDE

#include "HttpRequest.h"
class Buffer;

class HttpContext {
public:
    enum HttpRequestParseState {
        kExpectRequestLine, // 期望解析请求行, 获取请求方法, url, http版本信息
        kExpectHeaders,     // 期望解析请求头
        kExpectBody,        // 期望解析请求正文
        kGotAll,            // 解析完毕
    };

    HttpContext();
    // 出错会返回false
    bool parseRequest(Buffer* buf, Timestamp receiveTime);

    bool gotAll() const;
    void reset();
    
    const HttpRequest& request() const;
    HttpRequest& request();

private:
    bool processRequestLine(const char* begin, const char* end);

    HttpRequestParseState state_;
    HttpRequest request_;
};


#endif // MUDUO_NET_HTTPCONTEXT_INCLUDE
