#include"EventLoop.h"
#include"Poller.h"
#include"../Log/Logger.h"
#include"Channel.h"
#include"Thread.h"
#include <sys/eventfd.h>
#include<unistd.h>

const int kPollTimeMs = 10000;
int createEventfd(){
  int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0)
  {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return evtfd;    
}

EventLoop::EventLoop():
quit_(false),
thread_(new Thread(std::bind(&EventLoop::loop,this),"")),
poller_(new Poller),
wakeupFd_(createEventfd()),
wakeupChannel_(new Channel(this, wakeupFd_)){
    LOG_DEBUG << "EventLoop created " << this << " in thread " << CurrentThread::tid();
    wakeupChannel_->setReadCallBack(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
  LOG_DEBUG << "EventLoop " << this << " destructs in thread " << CurrentThread::tid();
            
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  close(wakeupFd_);
}

void EventLoop::startEventLoopThread(){
  thread_->start();
}


void EventLoop::loop()
{

  quit_ = false;  
  LOG_DEBUG << "EventLoop " << this << " start looping";

  while (!quit_)
  {
    actives_.clear();
    poller_->startPoll(kPollTimeMs, &actives_);

    //eventHandling_ = true;
    for (Channel* channel : actives_)
    {
      channel->handleEvent();
    }
    //eventHandling_ = false;
    doPendingFunctors();
  }
  LOG_DEBUG << "EventLoop " << this << " stop looping";
}

void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors;

  {
  MutexLockGuard lock(mutex_);
  //缩小临界区
  functors.swap(pendingFunctors_);
  }

  for (const Functor& functor : functors)
  {
    functor();
  }

}

void EventLoop::quit()
{
  quit_ = true;
  //避免loop阻塞在poller上
  wakeup();

}


void EventLoop::queueInLoop(Functor cb)
{
  {
  MutexLockGuard lock(mutex_);
  pendingFunctors_.push_back(std::move(cb));
  }
  //if()
  wakeup();
}
void EventLoop::wakeup()
{
  uint64_t one = 1;
  ssize_t n = write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n =read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::updateChannel(Channel* channel)
{
  poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel* channel)
{
  poller_->removeChannel(channel);
}