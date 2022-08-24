#ifndef XTIMERQUEUE_H
#define XTIMERQUEUE_H

#include"Timestamp.h"
#include"Channel.h"
#include<queue>
#include<vector>
#include<functional>
#include<memory>
class Timer;
class EventLoop;
//管理定时器，采用set，每次设置唤醒时间为定时器中最小的超时时间，保证唤醒时一定有定时器超时
class TimerQueue{
    
    typedef std::function<void()>TimerCallback;
    public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    //构造新的定时器并加入set
    void addTimer(TimerCallback cb,Timestamp when,double interval);

    private:
    typedef std::shared_ptr<Timer>TimerPtr;
    typedef std::pair<Timestamp,TimerPtr>Entry;
    //使用小根堆来管理定时器，堆顶超时时间作为timer_fd的唤醒时间
    struct Cmp{bool operator()(const Entry& a,const Entry& b){return b.first<a.first;}};
    typedef std::priority_queue<Entry,std::vector<Entry>,Cmp>TimerList;
    
    void handleRead();
    //定时回调,获取超时的定时器，以vector返回
    std::vector<Entry>getExpired(Timestamp now);
    //重新设置超时定时器，只有repeat定时器可以成功设置
    void reset(const std::vector<Entry>&expired,Timestamp now);

    //加入新的定时器
    bool insert(const TimerPtr& timer);

    EventLoop* loop_;
    //通过设置timerfd唤醒poller
    const int timerfd_;
    //唤醒后通过channel执行回调
    Channel timerfdChannel_;
    //管理timer
    TimerList timers_;

};

#endif