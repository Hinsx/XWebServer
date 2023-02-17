#include "Channel.h"
#include <poll.h>
#include <iostream>
#include "EventLoop.h"
#include"../Log/Logger.h"

#ifdef MYTRACE
#include <iostream>
using std::cout;
#endif

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd) : loop_(loop),
                                            fd_(fd),
                                            index_(-1),
                                            revent_(0),
                                            event_(0),
                                            tied_(false)
{
}
void Channel::update()
{
    loop_->updateChannel(this);
}
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::handleEvent()
{
    std::shared_ptr<void> guard;
    if (tied_)
    {
        //尝试升级
        guard = tie_.lock();
        //对象仍然存活
        if (guard)
        {
            handleEventWithGuard();
        }
        //对象死亡
    }
    else
    {
        handleEventWithGuard();
    }
}

/*
POLLRDHUP will be set when the other end has called shutdown(SHUT_WR) or when this end has called shutdown(SHUT_RD),
but the connection may still be alive in the other direction.

net/ipv4/tcp.c：
        if (sk->sk_shutdown == SHUTDOWN_MASK || state == TCP_CLOSE)
                mask |= EPOLLHUP;
        if (sk->sk_shutdown & RCV_SHUTDOWN)
                mask |= EPOLLIN | EPOLLRDNORM | EPOLLRDHUP;

SHUTDOWN_MASK is RCV_SHUTDOWN|SEND_SHUTDOWN. RCV_SHUTDOWN is set when a FIN packet is received, 
and SEND_SHUTDOWN is set when a FIN packet is acknowledged by the other end, and the socket moves to the FIN-WAIT2 state.

POLLRDHUP only works on sockets, not on fifos/pipes or ttys.

总结：
POLLHUP触发条件：无需设置event，当收到对面FIN 并且 本地发送FIN被对面ACK，意味着连接已经可以断开(close)了（双方不再发送数据，没有存在的意义）

POLLRDHUP触发条件：需要手动设置event，当 对面close/调用shutdown(SHUT_WR)/本地调用shutdown(SHUT_RD) 导致tcp接收缓冲区不能再收到新数据了（但是已经在缓冲区中的可以读）
        
由于POLLHUP触发需要本地发送FIN，只能通过调用shutdown(SHUT_WR)/close实现，所以只有先处理对端发过来的FIN（返回FIN），才会出现POLLHUP
不需要特意设置POLLRDHUP来感知对端挂起，因为只要接收到FIN报文一定会触发POLLIN事件（read=0)，并且会一直触发，直到连接close
*/

void Channel::handleEventWithGuard()
{
    LOG_TRACE<<"Ready to process event.";
    //文件描述没有打开
    if (revent_ & POLLNVAL)
    {
        //日志输出警告，文件描述符不是一个打开的文件
    }

    if ((revent_ & POLLHUP) && !(revent_ & POLLIN))
    {

        if (closeCallBack)
            closeCallBack();
    }
    if (revent_ & (POLLERR | POLLNVAL))
    {
        if (errorCallBack)
            errorCallBack();
    }

    if (revent_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (readCallBack)
            readCallBack();
    }
    if (revent_ & POLLOUT)
    {
        if (writeCallBack)
            writeCallBack();
    }
}
void Channel::remove()
{
    loop_->removeChannel(this);
}
