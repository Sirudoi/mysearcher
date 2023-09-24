#include "HttpResponse.h"
#include "../../muduo_include/Buffer.h"
#include <string.h>

HttpResponse::HttpResponse(bool close)
    : statusCode_(kUnknown), closeConnection_(close) {}

/**
 * @brief       设置响应报文状态码
 * @param code  状态码
 */
void HttpResponse::setStatusCode(HttpStatusCode code) {
    statusCode_ = code;
}

/**
 * @brief           设置响应报文信息
 * @param message   信息
 */
void HttpResponse::setStatusMessage(const std::string& message) {
    statusMessage_ = message;
}

/**
 * @brief    设置连接状态
 * @param on 状态
 */
void HttpResponse::setCloseConnection(bool on) {
    closeConnection_ = on;
}

/**
 * @brief 获取链接状态
 */
bool HttpResponse::closeConnection() const {
    return closeConnection_;
}

/**
 * @brief               设置Content-Type字段
 * @param contentType   需要设置的Content-Type字段值
 */
void HttpResponse::setContentType(const std::string& contentType) {
    addHeader("Content-Type", contentType);
}

/**
 * @brief       设置报头值
 * @param key   key值
 * @param value value值
 */
void HttpResponse::addHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

/**
 * @brief       设置body报文
 * @param body  对应报文
 */
void HttpResponse::setBody(const std::string& body) {
    body_ = body;
}

/**
 * @brief        构建完整http报文, 并将报文内容写入到指定缓冲区中
 * @param output 所选缓冲区指针
 */
void HttpResponse::appendToBuffer(Buffer* output) const {
    char buf[32];
    // 构建响应头 HTTP/1.1 200 OK
    snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", statusCode_);
    output->append(buf, strlen(buf));
    output->append(statusMessage_);
    output->append("\r\n", strlen("\r\n"));

    if (closeConnection_) {
        // 断开连接
        output->append("Connection: close\r\n");
    } 
    else {
        // 未断开连接, 设置Connection为Keep-Alive, 设置Content-Length字段
        snprintf(buf, sizeof(buf), "Content-Length: %zd\r\n", body_.size());
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n");
    }

    // 构建http报头
    for (const auto& header : headers_) {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }

    // 插入空行
    output->append("\r\n");
    // 插入http报文
    output->append(body_);
}