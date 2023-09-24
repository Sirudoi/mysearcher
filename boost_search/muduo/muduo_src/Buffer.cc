#include <assert.h>
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
#include <algorithm>

#include "../muduo_include/Buffer.h"

// http协议每一行的结尾
const char Buffer::kCRLF[] = "\r\n";

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
        // len == 可读取长度, 说明本次读取全部读完, 复位两个索引位置
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
 * @brief     更新可读取位置到end位置
 * @param end 需要更新的位置
 */
void Buffer::retrieveUntil(const char* end) {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
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
void Buffer::ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
        makeSpace(len);  // 扩容
    }
}

/**
 * @brief 把[data, data+len]内存上的数据添加到writable缓冲区当中
 */
void Buffer::append(const char* data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
}

/**
 * @brief 把传入的字符串追加到缓冲区中
 */
void Buffer::append(const std::string& data) {
    append(data.data(), data.size());
}

/**
 * @brief       确认len字节数据是否能写入缓冲区, 能写入则将可读取数据前移动,
 * 否则扩容
 * @param len   len字节数据
 */
void Buffer::makeSpace(size_t len) {
    /**
     * | kCheapPrepend | 已读取 | 待读取 | 可写入 |
     * | kCheapPrepend | 已读取 | 待读取 |  len          |
     **/

    // prependableBytes = | kCheapPrepend | 已读取 |
    // writableBytes    = | 可写入 |
    // 判断条件等价      = | 已读取 | 可写入 | < len
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
        // 如果复用     | 已读取 | 可写入 |两段空间, 都无法写入len字节, 则扩容
        // 直接向后扩容 | 已读取 | 待读取 | 可写入 | -> | 已读取 | 待读取 | len
        // |
        buffer_.resize(writerIndex_ + len);
    }
    // | 已读取 | 可写入 | > len
    // 则回收| 已读取 |的缓冲区
    else {
        size_t readable = readableBytes();
        // std::copy(InputIterator first, InputIterator last, OutputIterator
        // result) 将待读取部分的内容, 拷贝到初始位置处 | kCheapPrepend | 已读取
        // | 待读取 | 可写入 |  --> | kCheapPrepend | 待读取 |            可写入
        // |
        std::copy(begin() + readerIndex_, begin() + writerIndex_,
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
ssize_t Buffer::readFd(int fd, int* saveErrno) {
    // 栈额外空间，用于从套接字往出读时，当buffer_暂时不够用时暂存数据，待buffer_重新分配足够空间后，在把数据交换给buffer_。
    char extrabuf[65536] = {0};  // 栈上内存空间 65536/1024 = 64KB

    /*
    struct iovec {
        ptr_t iov_base; //
    iov_base指向的缓冲区存放的是readv所接收的数据或是writev将要发送的数据 size_t
    iov_len; // iov_len在各种情况下分别确定了接收的最大长度以及实际写入的长度
    };
    */

    // 使用iovec分配两个连续的缓冲区
    struct iovec vec[2];
    const size_t writable = writableBytes();  // buffer缓冲区可读取大小

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
        append(extrabuf,
               n - writable);  // 剩余n - writable在额外栈空间的缓冲区中,
                               // 追加到buffer
    }
    return n;
}

/**
 * @brief               向内核缓冲区写数据
 * @param fd            写入的fd
 * @param saveErrno     本次操作的errno
 * @return ssize_t      写入的实际字节数
 */
ssize_t Buffer::writeFd(int fd, int* saveErrno) {
    // 上层往outputBuffer_写数据, 写入后将该buffer | 可读取 | 写入到内核缓冲区
    // 内核往outputBuffer_读数据, 读取的数据就是outputBuffer_缓冲区 | 可读取 |
    // 的部分 peek() ——> readableBytes位置的地址
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0) {
        *saveErrno = errno;
    }
    return n;
}

/**
 * @brief               寻找start指针指向字符串第一个出现的的\r\n的位置
 * @param start         其实字符串
 * @return const char*
 * 返回\r\n第一次出现的位置，失败返回beginWrite()即可写入的第一个位置
 */
const char* Buffer::findCRLF(const char* start) const {
    // 可读取地址 < start
    assert(peek() <= start);
    // 可写入地址 < start
    assert(start <= beginWrite());

    /**
     * std::search()函数原型如下
     * template<class ForwardIt1, class ForwardIt2>
     * ForwardIt1 search(ForwardIt1 first1,
     *                   ForwardIt1 last1,
     *                   ForwardIt2 first2,
     *                   ForwardIt2 last2);
     * 查找一个字符串中另一个字符串第一次出现的位置
     * 找到返回出现的位置的迭代器, 未找到返回last1
     */

    // | 已读取 | 待读取 | 可写入 | 从 [start, 可写入 -
    // 1]位置，查找\r\n第一次出现 成功返回\r\n第一次出现位置，失败返回可写入位置
    const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? NULL : crlf;
}

/**
 * @brief 从缓冲区peek()位置查找第一次出现\r\n的位置, peek()即可读取位置
 * @return const char* 第一个\r\n出现的位置
 */
const char* Buffer::findCRLF() const {
    const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? NULL : crlf;
}