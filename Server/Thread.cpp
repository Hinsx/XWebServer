#include "Thread.h"
#include"CurrentThread.h"
#include"../Log/Logger.h"
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <iostream>
namespace detail
{
    //获取线程id
    pid_t gettid()
    {
        return static_cast<pid_t>(::syscall(SYS_gettid));
    }

    /*
    如果直接传递threadpoll的指针作为pthread_create的第四个参数，头文件会相互包含
    采用间接的方式实现
    */
    struct ThreadData
    {
        typedef std::function<void()> ThreadFunc;
        // public:
        ThreadData(ThreadFunc func,std::string n):
        data_func(func),
        data_name(n)
        {}
        void threadRun() { data_func(); }
        // private:
        // pthread_t data_pthreadId;
        ThreadFunc data_func;
        std::string data_name;
    };

    void *startThread(void *arg)
    {
        ThreadData *data = (ThreadData *)arg;
        //记录当前线程的tid
        CurrentThread::tid();
        //记录当前线程名称（xThread...)
        CurrentThread::t_threadName = data->data_name.c_str();

        LOG_DEBUG<<"New thread named "<<CurrentThread::t_threadName<<" create successfully.";
        data->threadRun();
        delete data;
        return nullptr;
    }
}

//通过此类记录主线程的id
class ThreadNameInitializer
{
 public:
  ThreadNameInitializer()
  {
    CurrentThread::t_threadName = "main";
    CurrentThread::tid();
  }
};

//只有主线程会创建init实例，从而记录主线程信息
ThreadNameInitializer init;

//在获取tid时，首先查看缓存tid，避免多次系统调用获取id
void CurrentThread::cacheTid()
{
  if (t_cachedTid == 0)
  {
    t_cachedTid = detail::gettid();
    t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
  }
}

//判断当前线程是否为主线程，主线程的线程id与进程id（线程组id）相同
bool CurrentThread::isMainThread()
{
  return tid() == ::getpid();
}

Thread::Thread(ThreadFunc func,std::string n) : joined_(false),
                                  started_(false),
                                  pthreadId_(0),
                                  func_(func),
                                  name_(n)
{
  setDefaultName();
}

std::atomic_int64_t Thread::number;

void Thread::setDefaultName(){
  if(name_.empty()){
      char buf[32];
      snprintf(buf, sizeof buf, "xThread%ld", number.fetch_add(1));
      name_ = buf;
  }
}

void Thread::start(bool run)
{
    started_=true;
    detail::ThreadData *data = new detail::ThreadData(func_,name_);
    //sleep(50);
    if (pthread_create(&pthreadId_, nullptr, detail::startThread, data) != 0)
    {
        //输出线程创建错误信息
        LOG_SYSFATAL<<"New thread create failed.";
        started_ = false;
        delete data;
    }
}
Thread::~Thread()
{
    if (started_&& !joined_)
    {
        pthread_detach(pthreadId_);
    }
}