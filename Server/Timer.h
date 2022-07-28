#ifndef XTIMER_H
#define XTIMER_H
#include<functional>
#include"Timestamp.h"
//定时器类，封装了超时执行的回调函数,用于执行连接shutdown
class Timer{

    typedef std::function<void()>TimerCallback;
    public:
    Timer(TimerCallback cb,Timestamp when,double interval):
    callback_(cb),
    expiration_(when),
    interval_(interval),
    //要设置大于0.0，否则interval_隐式转型为int
    repeat_(interval_>0.0)
    {}
    Timer():interval_(0),repeat_(false){}
    ~Timer();
    void run()const{
        callback_();
    }
    Timestamp expiration()const{return expiration_;}
    //eventloop runevery函数的基础
    bool repeat()const{return repeat_;}

    //定时器超时后重新设置
    void restart(Timestamp now);

    private:
    const TimerCallback callback_;
    //到期绝对时间
    Timestamp expiration_;
    //重复设置间隔，用于在超时时重新设置定时器，timerfd使用
    const double interval_;
    //是否设置了间隔
    const bool repeat_;
    
};

#endif