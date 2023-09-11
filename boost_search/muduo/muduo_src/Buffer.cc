#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

#include "../muduo_include/Buffer.h"

/**
 * @brief       读取数据之后更改readerIndex_位置
 * @param len   len长度
 */
void Buffer::retrieve(size_t len) {
    if (len < readableBytes()) {
        // len < 可读取长度, 说明此次只读取一部分, 则更改readerIndex_索引位置
        readerIndex_ += len;
    }
    else {
        // len == 可读取长度, 直接复位
        retrieveAll();
    }
}

/**
 * @brief 复位可读取索引和可写入索引
 */
void Buffer::retrieveAll() {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
}

/**
 * @brief               读取缓冲区所有可读取数据
 * @return std::string  读取的内容, 以string返回
 */
std::string Buffer::retrieveAllAsString() {
    return retrieveAsString(readableBytes()); 
}

/**
 * @brief               从缓冲区读取数据, 获取读取的内容
 * @param len           读取的长度 
 * @return std::string  读取的具体内容, 以string返回
 */
std::string Buffer::retrieveAsString(size_t len) {
    std::string result(peek(), len);
    // 更新索引
    retrieve(len);
    return result;
}

/**
 * @brief       检查缓冲区剩余容量是否足够写入, 不够则扩容
 * @param len   需要写入的长度
 */
void ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
        makeSpace(len); // 扩容
    }
}

/**
 * @brief 把[data, data+len]内存上的数据添加到writable缓冲区当中
 */
void append(const char *data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data+len, beginWrite());
    writerIndex_ += len;
}

/**
 * @brief       确认len字节数据是否能写入缓冲区, 能写入则将可读取数据前移动, 否则扩容
 * @param len   len字节数据
 */
void makeSpace(size_t len) {
    /**
    * | kCheapPrepend | 已读取 | 待读取 | 可写入 |
    * | kCheapPrepend | 已读取 | 待读取 |  len          |
    **/

    // prependableBytes = | kCheapPrepend | 已读取 |
    // writableBytes    = | 可写入 |
    // 判断条件等价      = | 已读取 | 可写入 | < len
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
        // 如果复用     | 已读取 | 可写入 |两段空间, 都无法写入len字节, 则扩容
        // 直接向后扩容 | 已读取 | 待读取 | 可写入 | -> | 已读取 | 待读取 |        len         |
        buffer_.resize(writerIndex_ + len);
    }
    // | 已读取 | 可写入 | > len
    // 则回收| 已读取 |的缓冲区
    else {
        size_t readable = readableBytes();
        // std::copy(InputIterator first, InputIterator last, OutputIterator result)
        // 将待读取部分的内容, 拷贝到初始位置处
        // | kCheapPrepend | 已读取 | 待读取 | 可写入 |  --> | kCheapPrepend | 待读取 |            可写入          | 
        std::copy(begin() + readerIndex_,
                    begin() + writerIndex_,
                    begin() + kCheapPrepend);
        // 重置read, write索引
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
    }
}

/**
 * @brief: 从内核缓冲区读取数据

 * 从socket读到缓冲区的方法是使用readv先读至buffer_，
 * Buffer_空间如果不够会读入到栈上65536个字节大小的空间，然后以append的
 * 方式追加入buffer_。既考虑了避免系统调用带来开销，又不影响数据的接收。
 **/
ssize_t Buffer::readFd(int fd, int *saveErrno) {
    // 栈额外空间，用于从套接字往出读时，当buffer_暂时不够用时暂存数据，待buffer_重新分配足够空间后，在把数据交换给buffer_。
    char extrabuf[65536] = {0}; // 栈上内存空间 65536/1024 = 64KB

    /*
    struct iovec {
        ptr_t iov_base; // iov_base指向的缓冲区存放的是readv所接收的数据或是writev将要发送的数据
        size_t iov_len; // iov_len在各种情况下分别确定了接收的最大长度以及实际写入的长度
    };
    */

    // 使用iovec分配两个连续的缓冲区
    struct iovec vec[2];
    const size_t writable = writableBytes(); // buffer缓冲区可读取大小

    // 第一块缓冲区，指向可写空间
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    // 第二块缓冲区，指向额外栈空间
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    // writable小于64kb, 会使用两块缓冲区, 最多能读取128kb数据
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    // 如果writable缓冲区小于额外栈空间, 使用两块缓冲区读, 否则使用一块
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0) {
        *saveErrno = errno;
    }
    // buffer缓冲区够读取, 直接更新writerIndex_索引
    else if (n <= writable) {
        writerIndex_ += n;
    }
    // buffer缓冲区写满了还是没读完, 借助了额外栈空间读取
    else {
        writerIndex_ = buffer_.size();  // 此时buffer读满了, 更新writerIndex_
        append(extrabuf, n - writable); // 剩余n - writable在额外栈空间的缓冲区中, 追加到buffer
    }
    return n;
}

/**
 * @brief               向内核缓冲区写数据
 * @param fd            写入的fd
 * @param saveErrno     本次操作的errno
 * @return ssize_t      写入的实际字节数
 */
ssize_t Buffer::writeFd(int fd, int *saveErrno) {
    // 上层往outputBuffer_写数据, 写入后将该buffer | 可读取 | 写入到内核缓冲区
    // 内核往outputBuffer_读数据, 读取的数据就是outputBuffer_缓冲区 | 可读取 | 的部分
    // peek() ——> readableBytes位置的地址
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0) {
        *saveErrno = errno;
    }
    return n;
}