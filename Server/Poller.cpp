#include <assert.h>
#include <string.h>
#include <iostream>
#include "Poller.h"
#include "Channel.h"
#include "../Log/Logger.h"

static_assert(EPOLLIN == POLLIN, "EPOLLIN == POLLIN");
static_assert(EPOLLPRI == POLLPRI, "EPOLLPRI == POLLPRI");
static_assert(EPOLLOUT == POLLOUT, "EPOLLOUT == POLLOUT");
static_assert(EPOLLOUT == POLLOUT, "EPOLLRDHUP == POLLRDHUP");
static_assert(EPOLLOUT == POLLOUT, "EPOLLERR == POLLERR");
static_assert(EPOLLOUT == POLLOUT, "EPOLLHUP == POLLHUP");

// Poller::Poller(EventLoop *loop, bool epoll) : loop_(loop),
//                                               events(eventsSize),
//                                               mode(epoll ? EPOLL : POLL)
// {
//     if (mode == EPOLL)
//     {
//         epollfd = epoll_create(5);
//     }
//     LOG_DEBUG << "Poller init success!";
// }
Poller::Poller(bool epoll) :
                                              events(eventsSize),
                                              mode(epoll ? EPOLL : POLL)
{
    if (mode == EPOLL)
    {
        epollfd = epoll_create(5);
    }
    LOG_DEBUG << "Poller init success!";
}
void Poller::startPoll(int timeoutMs, ActiveChannelList *list)
{
    //采用poll方式
    if (mode == POLL)
    {
        // std::cout<<"fds' size:"<<fds.size()<<",start polling...\n";
        LOG_TRACE << "Start polling."
                  << "Event scoure size = " << fds.size();
        int num = poll(&(*fds.begin()), fds.size(), timeoutMs);
        if (num < 0)
        {
            LOG_ERROR << "Poll error.";
        }
        if (num > 0)
        {
            LOG_TRACE<<num<<" events happend.Ready to fill.";
            fillActiveChannelList(num, list);
        }
    }
    //采用epoll方式
    if (mode == EPOLL)
    {
        LOG_TRACE << "Start epolling.";

        int num = epoll_wait(epollfd, &(events[0]), events.size(), timeoutMs);
        if (num < 0)
        {
            LOG_ERROR << "EPoll error.";
        }
        if (num > 0)
        {
            fillActiveChannelList(num, list);
        }
    }
}
void Poller::fillActiveChannelList(int num, ActiveChannelList *list)
{
    //从fds中寻找活跃的channel
    if (mode == POLL)
    {
        int counter = 0;
        for (auto iter = fds.begin(); iter != fds.end() && num>0; iter++)
        {
            //LOG_TRACE<<"Fd = "<<iter->fd<<",revents = "<<iter->revents;
            if (iter->revents > 0)
            {
                num--;
                ChannelMap::iterator it = channels.find(iter->fd);
                assert(it != channels.end());
                Channel *channel = it->second;
                assert(it != channels.end());
                channel->set_revent(iter->revents);
                list->push_back(channel);
            }
        }
    }
    //直接在epoll_event中构建channel
    if (mode == EPOLL)
    {
        for (int i = 0; i < num; i++)
        {
            Channel *channel = static_cast<Channel *>(events[i].data.ptr);
            assert(channel != nullptr);
            channel->set_revent(events[i].events);
            list->push_back(channel);
        }
    }
    LOG_TRACE << list->size() << " events happend.";
}
void Poller::updateChannel(Channel *channel)
{   
    LOG_TRACE<<"modify/add channel,fd="<<channel->fd();
    if (mode == POLL)
    {
        // channel修改已经存在fd，统一事件源需要修改
        if (channel->index() >= 0)
        {
            LOG_TRACE << "Channel " << channel->fd() << " modify.";
            auto iter = channels.find(channel->fd());
            assert(iter != channels.end());
            assert(channels[channel->fd()] == channel);
            pollfd &pfd = fds[channel->index()];
            assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);
            pfd.events = static_cast<short>(channel->event());
            pfd.fd = channel->fd();
            pfd.revents=0;
            //暂时不感兴趣,不希望监听任何事件
            if (channel->isNoneEvent())
            {
                pfd.fd = -channel->fd() - 1; //不感兴趣直接设置为负数
            }
        }
        //新增channel，记录映射，修改统一事件源
        else
        {
            LOG_TRACE << "Channel " << channel->fd() << " add.";
            auto iter = channels.find(channel->fd());
            assert(iter == channels.end());
            pollfd pfd;
            pfd.events = static_cast<short>(channel->event());
            pfd.fd = channel->fd();
            pfd.revents = 0;
            fds.push_back(pfd);
            int index = static_cast<int>(fds.size()) - 1;
            channel->set_index(index);
            channels[pfd.fd] = channel;
        }
    }
    if (mode == EPOLL)
    {
        // channel修改已经存在fd，统一事件源需要修改
        auto iter = channels.find(channel->fd());
        //已经记录fd-channel映射，fd已注册于内核事件源
        if (channel->index() == kAdded)
        {
            //修改事件源内的fd事件即可
            LOG_TRACE << "Channel " << channel->fd() << " modify.";
            epoll_event event;
            bzero(&event, sizeof event);
            event.events = channel->event();
            //在epoll返回event时快速寻找channel指针进行填充，如果选择fd成员，则还需要查询map
            event.data.ptr = channel;
            //暂时不感兴趣,不希望监听任何事件
            if (channel->isNoneEvent())
            {
                int ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, channel->fd(), &event);
                //映射仍然记录，但是从事件源中删除
                channel->set_index(kDelete);
            }
            else
            {
                int ret = epoll_ctl(epollfd, EPOLL_CTL_MOD, channel->fd(), &event);
                assert(ret >= 0);
            }
        }
        //新增channel
        else
        {
            //若是因为不感兴趣从事件源中删除的
            if (channel->index() == kDelete)
            {
                assert(iter != channels.end());
                assert(iter->second == channel);
            }
            if (channel->index() == kNew)
            {
                assert(iter == channels.end());
                channels[channel->fd()] = channel;
            }
            LOG_TRACE << "Channel " << channel->fd() << " add.";
            epoll_event event;
            bzero(&event, sizeof event);
            event.events = channel->event();
            event.data.ptr = channel;
            channel->set_index(kAdded);
            int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, channel->fd(), &event);
            if (ret < 0)
            {
                LOG_SYSERR << "Epoll add fd failure.\n";
            }
            assert(ret >= 0);
        }
    }
}
void Poller::removeChannel(Channel *channel)
{
    LOG_TRACE<<"remove channel,fd="<<channel->fd();
    if (mode == POLL)
    {
        int fd = channel->fd();
        auto iter = channels.find(fd);
        assert(iter != channels.end());
        assert(channels[channel->fd()] == channel);
        //先注销再移除(HttpConnection.cpp connectionDestory)
        assert(channel->isNoneEvent());
        int index = channel->index();
        assert(0 <= index && index < static_cast<int>(fds.size()));
        if (static_cast<size_t>(index) == fds.size() - 1)
        {
            
            fds.pop_back();
        }
        else
        {
            int fdAtEnd = fds.back().fd;
            iter_swap(fds.begin() + index, fds.end() - 1);

            //如果遗漏此判断，将导致段错误，因为channels[fdAtEnd]是空指针
            if(fdAtEnd<0){
                fdAtEnd=-fdAtEnd-1;
            }

            channels[fdAtEnd]->set_index(index);
            fds.pop_back();
        }
        size_t n=channels.erase(fd);
        assert(n==1);
    }
    if (mode == EPOLL)
    {   
        assert(channel->index()==kDelete||channel->index()==kAdded);
        int fd = channel->fd();
        epoll_event event;
        bzero(&event, sizeof event);
        event.data.ptr = channel;
        event.events = channel->event();
        if(channel->index()==kAdded){
            int ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
            //连接handleClose时已经设置disableAll，此时再次调用connectionDestroy remove，则会重复DEL，正常
            //assert(ret >= 0);            
        }

        channels.erase(fd);
    }
}