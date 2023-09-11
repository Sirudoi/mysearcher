#include "../muduo_include/Poller.h"
#include "../muduo_include/Channel.h"

Poller::Poller(EventLoop *loop)
    : loop_(loop) {

}

/**
 * @brief           判断某个Channel是否在poller中
 * @param channel   Channel对象
 */
bool Poller::hasChannel(Channel *channel) const {
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}