#ifndef MUDUO_NET_HTTPREQUEST_INCLUDE
#define MUDUO_NET_HTTPREQUEST_INCLUDE

#include <map>
#include <assert.h>
#include <stdio.h>

#include "../../muduo_include/Timestamp.h"

class HttpRequest {
public:
    // 请求方法
    enum Method {
        kInvalid,
        kGet, 
        kPost, 
        kHead, 
        kPut, 
        kDelete
    };

    // http版本
    enum Version {
        kUnknown, 
        kHttp10, 
        kHttp11
    };

    HttpRequest();
    ~HttpRequest();

    void setVersion(Version v);
    Version getVersion() const;

    bool setMethod(const char* start, const char* end);
    Method method() const { return method_; }
    const char* methodString() const;

    // 设置[start, end]这段迭代器区间为path
    void setPath(const char* start, const char* end) { path_.assign(start, end); }
    const std::string& path() const { return path_; }

    void setQuery(const char* start, const char* end) { query_.assign(start, end); }
    const std::string& query() const { return query_; }

    void setReceiveTime(Timestamp t) { receiveTime_ = t; }
    Timestamp receiveTime() const { return receiveTime_; }

    void addHeader(const char* start, const char* colon, const char* end);
    std::string getHeader(const std::string& field) const;
    const std::map<std::string, std::string>& headers() const;

    void swap(HttpRequest& that);

private:
    Method method_;                                 // 请求方法
    Version version_;                               // http版本
    std::string path_;                              // 请求路径
    std::string query_;                             // 请求参数
    Timestamp receiveTime_;                         // 接收时间戳
    std::map<std::string, std::string> headers_;    // 报头key-value值
};

#endif // MUDUO_NET_HTTPREQUEST_INCLUDE