#ifndef XTHREAD_H
#define XTHREAD_h
#include <pthread.h>
#include <functional>
#include<string>
#include<atomic>

//封装线程和基本操作，包括create/join/detach，利用pthread_t成员来管理线程的生命周期
class Thread
{
    //线程主循环
    typedef std::function<void()>ThreadFunc;
public:
    Thread(ThreadFunc func,std::string n);
    void start();
    void setstart(){started_=true;}
    bool stareted(){return started_;}
    int join(){
        joined_ = true;
        return pthread_join(pthreadId_, NULL);
    }
    ~Thread();
    static void* test(void*);
private:    
    void setDefaultName();
    bool joined_;
    bool started_;
    pthread_t pthreadId_;
    ThreadFunc func_;
    std::string name_;

    //设置默认线程名编号
    static std::atomic_int64_t number;
};
#endif