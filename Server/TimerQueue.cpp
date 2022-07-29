#include"TimerQueue.h"
#include"../Log/Logger.h"
#include"EventLoop.h"
#include"Timer.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include<assert.h>

int createTimerfd(){
    //以系统启动时间为基准
    int timerfd=timerfd_create(CLOCK_MONOTONIC,TFD_CLOEXEC|TFD_NONBLOCK);
    if(timerfd<0){
        LOG_SYSFATAL<<"Failed in timerfd_create";
    }
    return timerfd;
}
void readTimerfd(int timerfd,Timestamp now){
    long long result;
    ssize_t n=read(timerfd,&result,sizeof(result));
    LOG_TRACE<<"TimerQueue wake up.";
    if(n!=sizeof(result)){
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}
//设置timerfd唤醒的绝对时间，为expiration
void resetTimerfd(int timerfd,Timestamp expiration){
    itimerspec newValue;
    itimerspec oldValue;
    bzero(&newValue,sizeof(newValue));
    bzero(&oldValue,sizeof(oldValue));
    long long microseconds=expiration.microSeconds()-Timestamp::now().microSeconds();
    if(microseconds<100){
        //避免queue过早唤醒,导致busy loop
        microseconds=100;
    }
    //计算间隔
    timespec ts;
    ts.tv_sec=static_cast<time_t>(microseconds/Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec=static_cast<long>((microseconds%Timestamp::kMicroSecondsPerSecond)*1000);

    newValue.it_value=ts;
    int ret=timerfd_settime(timerfd,0,&newValue,&oldValue);
    if(ret){
        LOG_SYSERR << "Failed in timerfd_settime()";
    }
}

TimerQueue::TimerQueue(EventLoop* loop):
loop_(loop),
timerfd_(createTimerfd()),
timerfdChannel_(loop,timerfd_),
timers_(){
    timerfdChannel_.setReadCallBack(std::bind(&TimerQueue::handleRead,this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue(){
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    close(timerfd_);
}
void TimerQueue::addTimer(TimerCallback cb,Timestamp when,double interval){
    TimerPtr timer(new Timer(cb,when,interval));
    bool newEarliest=insert(timer);
    if(newEarliest){
        resetTimerfd(timerfd_,timer->expiration());
    }
}

void TimerQueue::handleRead(){
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_,now);
    std::vector<Entry>expired=getExpired(now);
    //执行定时器的超时回调
    for(const Entry& it:expired){
        it.second->run();
    }
    //重设repeatable的定时器
    reset(expired,now);
}

std::vector<TimerQueue::Entry>TimerQueue::getExpired(Timestamp now){
    std::vector<Entry>expired;
    //通过二分upper_bound寻找最后一个超时的(包括=now)定时器，以其为结尾将set中的超时定时器取出
    Timer tmp;
    Entry sentry(now,TimerPtr(&tmp,[](Timer* ptr){}));
    TimerList::iterator end=timers_.upper_bound(sentry);
    assert(end == timers_.end() || now < end->first);
    //使用back_inserter不必事先知道要复制的元素数量,这会顺带复制shareptr
    std::copy(timers_.begin(),end,back_inserter(expired));
    //定时器取出
    timers_.erase(timers_.begin(),end);
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired,Timestamp now){
    Timestamp nextExpire;
    for(const Entry& it:expired){
        if(it.second->repeat()){
            it.second->restart(now);
            insert(it.second);
        }
    }
    //将timequeue唤醒时间设置为最小的超时时间
    if(!timers_.empty()){
        nextExpire=timers_.begin()->second->expiration();
    }
    //当至少存在一个定时器，设置唤醒时间
    if(nextExpire.valid()){
        resetTimerfd(timerfd_,nextExpire);
    }
}

bool TimerQueue::insert(const TimerPtr& timer){
    bool newEarliest=false;
    Timestamp when=timer->expiration();
    TimerList::iterator it=timers_.begin();
    //新增的定时器最小，需要设置timequeue的唤醒时间
    if(it==timers_.end()||when<it->first){
        newEarliest=true;
    }
    std::pair<TimerList::iterator,bool>result=timers_.insert(Entry(when,timer));
    assert(result.second);
    return newEarliest;
}