#ifndef XPOLLER_H
#define XPOLLER_H
#include<vector>
#include<map>
#include<poll.h>
#include <sys/epoll.h>

class Channel;
class EventLoop;

//io复用抽象，有时间应该实现为一个pollbase虚基类，然后由poll和eopoll两种子类实现
class Poller{

    typedef std::vector<Channel*>ActiveChannelList;

    

    //记录fd到Channel的映射，在poll/epoll返回后根据fd在map中找到对应channel
    typedef std::map<int,Channel*>ChannelMap;
    

    //枚举类型
    enum pollmode{POLL=0,EPOLL};

    public:
    //Poller(EventLoop* EventLoop,bool epoll_=true);
    Poller(bool epoll_=true);
    //更新事件源,新增或者修改
    void updateChannel(Channel* channel);
    //开始监听统一事件源
    void startPoll(int timeoutMs,ActiveChannelList* list);
    //删除channel,从统一事件源和map中移除,当连接关闭时调用
    void removeChannel(Channel* channel);

    private:
    //监听返回，填充活跃的channel
    void fillActiveChannelList(int num,ActiveChannelList* list);

    //采用poll或是epoll的方式,0=poll,1=epoll,默认poll
    const pollmode mode;

    /*poll*/
    //统一事件源,使用poll模式会用到
    std::vector<pollfd>fds;
    //记录fd和channel的映射关系
    ChannelMap channels;
    
    /*epoll*/
    //内核事件表
    int epollfd;
    //存放epoll返回的活跃fd
    std::vector<epoll_event>events;
    //初始化events大小
    static const int eventsSize = 16;
    //新增fd时判断的依据(若不在事件源的原因是不感兴趣，那么跳过设置映射这一步)
    static int const kNew=-1;
    static int const kAdded=1;
    static int const kDelete=2;
    

    //EventLoop* loop_;
    
};

#endif