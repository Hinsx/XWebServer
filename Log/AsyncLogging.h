#ifndef XASYNCLOGGING_H
#define XASYNCLOGGING_H

#include"LogStream.h"
#include"../Server/Thread.h"
#include"../Server/Lock.h"
#include<string>
#include<vector>
#include<memory>
#include<atomic>

//收集前端信息，输出到磁盘
class AsyncLogging{

    public:
  AsyncLogging(const string& basename,
               int rollSize,
               int flushInterval = 3);

  ~AsyncLogging()
  {
    if (running_)
    {
      stop();
    }
  }

  void append(const char* logline, int len);

  //启动日志线程
  void start()
  {
    running_ = true;
    thread_.start();
  }
  //停止日志线程
  void stop() 
  {
    running_ = false;
    cond_.notify();
    thread_.join();
  }
    AsyncLogging(const AsyncLogging&)=delete;
    void operator=(const AsyncLogging&)=delete;
 private:

  void threadFunc();

  typedef detail::FixedBuffer<detail::kLargeBuffer> Buffer;
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  typedef BufferVector::value_type BufferPtr;

  //获取前端buffer的时间间隔，防止写入消息过少导致长时间没有信息写入日志文件，即刷新缓冲区
  const int flushInterval_;
  //日志线程停止变量
  std::atomic<bool> running_;
  //日志目标文件
  const string basename_;
  //单个日志文件记录量上限
  const off_t rollSize_;
  //异步日志线程
  Thread thread_;
  //前端后端使用互斥锁和条件变量同步
  MutexLock mutex_;
  Condition cond_;
  //前端使用的buffer
  BufferPtr currentBuffer_;
  //备用buffer，减少前端new的执行次数
  BufferPtr nextBuffer_;
  //前端写满一块buffer后传入vector
  BufferVector buffers_;

};

#endif
