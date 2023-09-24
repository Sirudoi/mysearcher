#ifndef INCLUDE_MUDUO_BUFFER_H
#define INCLUDE_MUDUO_BUFFER_H

#include <vector>
#include <string>
#include <algorithm>
#include <stddef.h>

class Buffer {
public:
    static const size_t kCheapPrepend = 8;      // 初始位置, 偏移8个字节用于解决粘包问题
    static const size_t kInitialSize = 1024;    // 初始buffer总长度

    explicit Buffer(size_t initalSize = kInitialSize)
        : buffer_(kCheapPrepend + initalSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {}

    // 获取缓冲区不同位置的长度
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }

    // 返回buffer可读取内容的首地址
    // 首元素地址 + readerIndex_ = 可读取位置的地址, 地址 + 1会偏移一个字节, 地址的单位是字节
    const char *peek() const { return begin() + readerIndex_; }

    // 更改索引
    void retrieve(size_t len);                      // 读取之后更改索引
    void retrieveAll();                             // 复位索引
    void Buffer::retrieveUntil(const char* end);    // 更新索引到end位置
 
    // 读取
    std::string retrieveAllAsString();
    std::string retrieveAsString(size_t len);

    void ensureWritableBytes(size_t len);
    void append(const char *data, size_t len);
    void append(const std::string& data);

    // vector底层数组可写入位置的地址
    char *beginWrite() { return begin() + writerIndex_; }
    const char *beginWrite() const { return begin() + writerIndex_; }

    // 读写数据
    ssize_t readFd(int fd, int *saveErrno);
    ssize_t writeFd(int fd, int *saveErrno);

    // 寻找\r\n标识符
    const char* findCRLF(const char* start) const;
    const char* findCRLF() const;


private:
    // vector底层数组首元素的地址
    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }

    // 扩容具体操作
    void makeSpace(size_t len);


    std::vector<char> buffer_;  // buffer采用一个字符数组
    size_t readerIndex_;        // 读位置索引
    size_t writerIndex_;        // 写位置索引

    static const char kCRLF[];  // http协议结尾的回车换行符 
};

#endif // INCLUDE_MUDUO_BUFFER_H