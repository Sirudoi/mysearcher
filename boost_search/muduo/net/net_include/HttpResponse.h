#ifndef MUDUO_NET_HTTPHTTPRESPONSE_INCLUDE
#define MUDUO_NET_HTTPHTTPRESPONSE_INCLUDE

#include <map>

class Buffer;

class HttpResponse {
public:
    // 响应报文状态
    enum HttpStatusCode {
        kUnknown,
        k200Ok = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404,
    };

    explicit HttpResponse(bool close);

    // Member Set
    void setStatusCode(HttpStatusCode code);
    void setStatusMessage(const std::string& message);
    void setCloseConnection(bool on);
    void setContentType(const std::string& contentType);

    // Member Change
    void addHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);

    // Member Get
    bool closeConnection() const;

    void appendToBuffer(Buffer* output) const;

private:
    std::map<std::string, std::string> headers_;    // 报头
    HttpStatusCode statusCode_;                     // 响应码
    // FIXME: add http version
    std::string statusMessage_;                     // 响应消息
    bool closeConnection_;                          // 连接状态
    std::string body_;                              // 报文
};

#endif  // MUDUO_NET_HTTPHTTPRESPONSE_INCLUDE