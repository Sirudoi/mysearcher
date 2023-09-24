#include "HttpRequest.h"

HttpRequest::HttpRequest()
    : method_(HttpRequest::kInvalid), version_(HttpRequest::kUnknown) {}

/**
 * @brief       设置本次请求的方法
 * @param start 起始迭代器
 * @param end   结束迭代器
 */
bool HttpRequest::setMethod(const char* start, const char* end) {
    assert(method_ == kInvalid);
    std::string m(start, end);

    if (m == "GET") {
        method_ = kGet;
    } else if (m == "POST") {
        method_ = kPost;
    } else if (m == "HEAD") {
        method_ = kHead;
    } else if (m == "PUT") {
        method_ = kPut;
    } else if (m == "DELETE") {
        method_ = kDelete;
    } else {
        method_ = kInvalid;
    }

    return method_ != kInvalid;
}

/**
 * @brief   返回请求方法的原生字符串
 * @return  const char* 的请求方法字符串
 */
const char* HttpRequest::methodString() const {
    const char* result = "UNKNOWN";
    switch (method_) {
        case kGet:
            result = "GET";
            break;
        case kPost:
            result = "POST";
            break;
        case kHead:
            result = "HEAD";
            break;
        case kPut:
            result = "PUT";
            break;
        case kDelete:
            result = "DELETE";
            break;
        default:
            break;
    }

    return result;
}

/**
 * @brief       从[start, end]这段字符串中, 解析出改行http报文的key和value,
 * 并保存起来
 * @param start 该行http报文起始位置
 * @param colon 该行http报文':'所在位置
 * @param end   该行http报文结束位置
 */
void HttpRequest::addHeader(const char* start,
                            const char* colon,
                            const char* end) {
    // 获得该行http报文的key值
    std::string field(start, colon);
    ++colon;
    /* isspace检查字符值是否为空白字符, 空格, 制表等
     * 这个操作是为了跳过键值之前或者之后的空格
     * 例如：key1:value 这种不用跳过
     * key2 : value2 这种需要跳过
     */
    while (colon < end && isspace(*colon)) {
        ++colon;
    }

    // 获得该行http报文的value值
    std::string value(colon, end);
    // 检查删除后导空格
    while (!value.empty() && isspace(value[value.size() - 1])) {
        value.resize(value.size() - 1);
    }
    headers_[field] = value;
}

/**
 * @brief               获取http报文中某个键对应的值
 * @param field         具体的键
 * @return std::string  传入键对应的值
 */
std::string HttpRequest::getHeader(const std::string& field) const {
    std::string result;
    std::map<std::string, std::string>::const_iterator it =
        headers_.find(field);
    if (it != headers_.end()) {
        result = it->second;
    }
    return result;
}

/**
 * @brief 返回http报文报头的map
 */
const std::map<std::string, std::string>& HttpRequest::headers() const {
    return headers_;
}

/**
 * @brief       交换两个报头的资源
 * @param that  对应报头
 */
void HttpRequest::swap(HttpRequest& that) {
    std::swap(method_, that.method_);
    std::swap(version_, that.version_);
    path_.swap(that.path_);
    query_.swap(that.query_);
    // TODO
    // receiveTime_.swap(that.receiveTime_);
    headers_.swap(that.headers_);
}
