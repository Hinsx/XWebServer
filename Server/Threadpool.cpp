#include "Threadpool.h"
#include "Channel.h"
#include "../Log/Logger.h"
#include "EventLoop.h"
#include <iostream>
int Threadpool::kmaxThreadNumber = 5;
EventLoop* Threadpool::kLoop =nullptr;
// int Threadpool::maxRequestNumber_=10000;

pthread_once_t Threadpool::ponce_ = PTHREAD_ONCE_INIT;
Threadpool *Threadpool::threadpool_ = nullptr;

Threadpool::Threadpool(EventLoop *loop) : loop_(loop),
                                          maxThreadNumber(kmaxThreadNumber),
                                          loopIndex(0)
// maxRequestNumber(maxRequestNumber_),
// poolName("xThread"),
// cond(mutex),
// stop(false)
{

    LOG_DEBUG << "Threadpool create successfully.";
}

void Threadpool::start()
{
    LOG_INFO<<"The number of thread is "<<maxThreadNumber;
    if(maxThreadNumber==0){
        return;
    }
    loops_.reserve(maxThreadNumber);
    for (int i =0; i < maxThreadNumber; i++)
    {
        loops_.emplace_back(new EventLoop);
        loops_[i]->startEventLoopThread();
    }
}
EventLoop *Threadpool::getNextLoop()
{
    EventLoop *loop = loop_;
    if (!loops_.empty())
    {
        if (static_cast<size_t>(loopIndex) >= loops_.size())
        {
            loopIndex = 0;
        }
        loop = loops_[loopIndex];
        loopIndex++;
    }
    else LOG_TRACE<<"Use base loop "<<loop;

    return loop;
}

Threadpool::~Threadpool()
{
}

// void* Threadpool::work(void* arg){
//     Threadpool* pool=(Threadpool*)arg;
//     pool->run();
//     return nullptr;
// }
// void Threadpool::run(){

//     while(!stop){
//         mutex.lock();
//         while(workqueue.empty()){
//             LOG_DEBUG<<"Thread sleep.";
//             cond.wait();
//         }
//         LOG_DEBUG<<"Thread wakeup.";
//         Channel* channel=workqueue.front();
//         workqueue.pop();
//         mutex.unlock();
//         channel->handleEvent();
//     }
// }

// bool Threadpool::append(Channel* channel){
//     mutex.lock();
//     //队列已满，只能等待下一次处理
//     if(workqueue.size()>=maxRequestNumber){
//         mutex.unlock();
//         return false;
//     }
//     //暂时不感兴趣,若监听器使用poll，可以防止一个fd同时被两个线程处理(但是可能监听到错误，有待修复)
//     channel->disableAll();
//     workqueue.push(channel);
//     //如果先解锁后通知，可能其他线程刚好获取锁然后取走唯一的请求，此时其他线程唤醒后发现队列为空，再次阻塞（虚假唤醒）
//     cond.notify();
//     mutex.unlock();
//     return true;
// }
