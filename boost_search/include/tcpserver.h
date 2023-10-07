#ifndef INCLUDE_TCPSERVER_H
#define INCLUDE_TCPSERVER_H

#ifndef EPOLL_ARRAY_SIZE
#define EPOLL_ARRAY_SIZE 1000
#endif

#ifndef EPOLL_MAX_EVENTS
#define EPOLL_MAX_EVENTS 1000
#endif


#include "./threadpool.h"
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace ns_tcpserver{

class TcpServer {
public:
    using Handler = std::function<void(const Request &req, const Response &resp)>;

    TcpServer();
    virtual ~TcpServer();

    TcpServer &Get(const char *pattern, Handler handler);
    TcpServer &Post(const char *pattern, Handler handler);

    bool listen(const char *host, int port, int socket_flags = 0);
    bool bind_to_port(const char *host, int port, int socket_flags = 0);
    bool start_to_listen();

    bool epoll_create(size_t size = 5000);
    bool epoll_add(int events, int fd);
    bool epoll_remove(int fd);

private:
    int epfd_;
    int svr_sock_;
    struct epoll_event eparr_[EPOLL_ARRAY_SIZE];
    bool is_running_;
};


}

#endif // INCLUDE_TCPSERVER_H