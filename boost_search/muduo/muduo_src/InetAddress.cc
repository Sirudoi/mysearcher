#include <strings.h>
#include <string.h>

#include "../muduo_include/InetAddress.h"

InetAddress::InetAddress(uint16_t port, std::string ip) {
    ::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = ::htons(port);
    addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());
}

/**
 * @brief 返回Ip
 * @return std::string 
 */
std::string InetAddress::toIp() const {
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

/**
 * @brief 返回ip:端口
 * @return std::string 
 */
std::string InetAddress::toIpPort() const {
    // ip:port
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = ::strlen(buf);
    uint16_t port = ::ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u", port);
    return buf;
}

/**
 * @brief 返回端口
 * @return uint16_t 
 */
uint16_t InetAddress::toPort() const {
    return ::ntohs(addr_.sin_port);
}