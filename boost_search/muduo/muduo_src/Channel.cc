#include <sys/epoll.h>

#include "../muduo_include/Channel.h"

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false) {

}

Channel::~Channel() {
    
}