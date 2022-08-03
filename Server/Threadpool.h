#ifndef XTHREADPOOL_H
#define XTHREADPOOL_H
#include"Lock.h"
#include<pthread.h>
#include<queue>
#include<vector>
#include<memory>
#include<string>
class Channel;
class EventLoop;

//单例模式封装线程池
class Threadpool{

    //typedef std::vector<pthread_t>ThreadList;
    //typedef std::queue<Channel*>RequestQueue;
    //static int maxRequestNumber_;

    public:
    //初始化线程池的线程（主线程）
    static EventLoop* kLoop;
    //线程池大小
    static int threadNum;
    //线程池大小的上限
    static int kMaxThreadNum;
    //eventloop的io复用模式
    static bool mode;
    //单例模式对外接口
    static Threadpool* init(EventLoop* loop){
        kLoop=loop;
        pthread_once(&ponce_,Threadpool::createThreapollInstance);
        return threadpool_;
    }
    static void setThreadNum(int num){
        num=num<0?0:(num>kMaxThreadNum?kMaxThreadNum:num);
        threadNum=num;
    }
    static void setPollMode(bool m){
        mode=m;
    }
    // static void set_maxRequestNumber(int num){
    //     maxRequestNumber_=num;
    // }
    ~Threadpool();
    //启动线程池
    void start();
    //分配EventLoop
    EventLoop* getNextLoop();

    //停止,待实现，需要保证正在处理的请求能完整处理。
    //void stop();
    
    Threadpool(const Threadpool& threadpool)=delete;
    Threadpool& operator=(const Threadpool& threadpool)=delete;
    //添加待处理的channel
    //bool append(Channel* channel);

    private:
    //构造函数在程序生命周期内只能调用一次
    Threadpool(EventLoop* loop);
    static void createThreapollInstance(){
        threadpool_=new Threadpool(kLoop);
    }
    //创建线程时传入的静态函数，固定格式,
    //static void* work(void*);
    //线程主循环
    //void run();

    static pthread_once_t ponce_;
    static Threadpool* threadpool_;
    EventLoop* loop_;
    //线程池中的线程数量
    int maxThreadNumber;
    //EventLoop循环编号
    int loopIndex;
    //请求队列中请求数量上限
    //int maxRequestNumber;
    //线程池名称
    //std::string poolName;
    //线程对象
    //std::vector<std::unique_ptr<Thread> >threads;
    std::vector<EventLoop*>loops_;
    //请求队列
    //RequestQueue workqueue;
    //互斥锁
    //MutexLock mutex;
    //条件变量
    //Condition cond;
    //线程池停止所有线程
    //bool stop;
};

#endif