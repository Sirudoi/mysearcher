#include "HttpContext.h"
#include "../../muduo_include/Buffer.h"

HttpContext::HttpContext() : state_(kExpectRequestLine) {}

/**
 * @brief 如果解析完毕返回true
 */
bool HttpContext::gotAll() const {
    return state_ == kGotAll;
}

/**
 * @brief 重置解析期望为请求行, 清空该Context内http请求报文内容
 */
void HttpContext::reset() {
    state_ = kExpectRequestLine;
    HttpRequest dummy;
    request_.swap(dummy);
}

/**
 * @brief 获取内部HttpRequest
 */
const HttpRequest& HttpContext::request() const {
    return request_;
}

/**
 * @brief 获取内部HttpRequest
 */
HttpRequest& HttpContext::request() {
    return request_;
}

/**
 * @brief       解析请求行
 * @param begin 请求行其实位置
 * @param end   请求行结束位置
 */
bool HttpContext::processRequestLine(const char* begin, const char* end) {
    bool succeed = false;
    // GET /hello.txt HTTP/1.1
    const char* start = begin;
    // 找第一个空格, 此时[start, space]位置的字符串就是请求方法
    const char* space = std::find(start, end, ' ');

    if (space != end && request_.setMethod(start, space)) {
        // 更新到下一个位置
        start = space + 1;
        space = std::find(start, end, ' ');
        // [start, space]为请求路径字符串
        if (space != end) {
            // 查看请求路径是否有?
            const char* question = std::find(start, space, '?');
            // 有? -> 有请求参数
            if (question != space) {
                // 设置请求路径和请求参数
                request_.setPath(start, question);
                request_.setQuery(question, space);
            }
            // 没有? -> 没有请求参数
            else {
                // 只设置请求路径
                request_.setPath(start, space);
            }
            start = space + 1;
            succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
            // 设置http版本
            if (succeed) {
                if (*(end - 1) == '1') {
                    request_.setVersion(HttpRequest::kHttp11);
                } else if (*(end - 1) == '0') {
                    request_.setVersion(HttpRequest::kHttp10);
                } else {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

/**
 * @brief               解析请求头
 * @param buf           该http报文所在缓冲区
 * @param receiveTime   接收时间
 * @return true         解析成功
 * @return false        解析出错
 */
bool HttpContext::parseRequest(Buffer* buf, Timestamp receiveTime) {
    bool ok = true;
    bool hasMore = true; // 解析完毕标志位
    while (hasMore) {
        // 解析请求行
        if (state_ == kExpectRequestLine) {
            // 找到请求头\r\n位置
            const char* crlf = buf->findCRLF();
            if (crlf) {
                // 解析请求头
                ok = processRequestLine(buf->peek(), crlf);
                if (ok) {
                    request_.setReceiveTime(receiveTime);
                    // 过掉\r\n位置, 将读取位置更新到下一行http请求
                    buf->retrieveUntil(crlf + 2);
                    // 更新期望为解析请求头
                    state_ = kExpectHeaders;
                } 
                else {
                    hasMore = false;
                }
            } 
            else {
                hasMore = false;
            }
        }
        // 解析请求头
        else if (state_ == kExpectHeaders) {
            // 获取单行请求的前后位置
            const char* crlf = buf->findCRLF();
            if (crlf) {
                // 找到 : 
                const char* colon = std::find(buf->peek(), crlf, ':');

                // 解析:左右的key和value, 添加到报头map中
                if (colon != crlf) {
                    request_.addHeader(buf->peek(), colon, crlf);
                } 
                else {
                    // 该行HTTP报文没有':'字符, 说明为空行, 即结束行
                    state_ = kGotAll;
                    hasMore = false;
                }
                buf->retrieveUntil(crlf + 2);
            } 
            else {
                hasMore = false;
            }
        } 
        else if (state_ == kExpectBody) {
            // FIXME:
        }
    }
    return ok;
}