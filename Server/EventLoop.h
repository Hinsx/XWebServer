 #ifndef XEVENTLOOP_H
#define XEVENTLOOP_H

#include<memory>
#include<vector>
#include <functional>
#include"Lock.h"

#include"CurrentThread.h"


class Thread;
class Poller;
class Channel;
class TimerQueue;

//工作线程创建，阻塞在poll/epoll上，负责管理http连接,包括进行监听，信息处理，定时器
class EventLoop{

    typedef std::vector<Channel*> ActiveChannelList;
    typedef std::function<void()> Functor;
    typedef std::function<void()> TimerCallback;
    public:
    EventLoop();
    void startEventLoopThread();
    ~EventLoop();
    //若当前调用函数的线程是创建loop的线程，则返回true
    //bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
    void loop();
    void quit();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

    //每隔interval秒触发一次定时事件，用于踢掉长连接
    void runEvery(double interval,TimerCallback cb);
    //加入回调函数，需要互斥锁保护functors
    //用于修改loop事件源，避免直接注册channel修改poller导致线程不安全，也避免为了等待poller唤醒而一直等待
    //线程不安全：
    //poller：fds，若一个连接修改channel时，刚刚获得channel引用，accptor新增channel需要push_back，导致扩容，引用失效
    //epoller:epoll_wait和epoll_ctl本身线程安全，但同时新增channel和删除channel仍然造成map失效（poller实际上有同样的问题）
    void queueInLoop(Functor);

    private:
    
    //激活fd，打断poller阻塞
    void wakeup();
    //处理eventfd
    void handleRead();

    //执行pendingFunctors_中的连接建立回调
    void doPendingFunctors();


    bool quit_;
    std::unique_ptr<Poller>poller_;
    ActiveChannelList actives_;
    //创建者的线程id
    //const pid_t threadId_;
    std::unique_ptr<Thread>thread_;
    //定时器堆
    std::unique_ptr<TimerQueue>timerQueue_;

    //通过监听wakeupFd_打断poller的阻塞，从而可以执行新到来的http连接建立回调
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    //存储连接建立（establish）回调
    mutable MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;
};

#endif