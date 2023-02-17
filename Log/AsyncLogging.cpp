#include"AsyncLogging.h"
#include"LogFile.h"
#include"Logger.h"
#include <stdio.h>
#include<assert.h>
#include<functional>

AsyncLogging::AsyncLogging(const string& basename,
                           int rollSize,
                           int flushInterval)
  : flushInterval_(flushInterval),
    running_(false),
    basename_(basename),
    rollSize_(rollSize),
    thread_(std::bind(&AsyncLogging::threadFunc, this), "xLogging"),
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_()
{
  currentBuffer_->bzero();
  nextBuffer_->bzero();
  buffers_.reserve(16);
  Logger::setOutput(std::bind(&AsyncLogging::append,this,std::placeholders::_1,std::placeholders::_2));
}

void AsyncLogging::append(const char* logline, int len)
{
  //后端也会争用buffer，互斥锁保护临界区
  MutexLockGuard lock(mutex_);
  if (currentBuffer_->avail() > len)
  {
    currentBuffer_->append(logline, len);
  }
  else
  {
    //unique_ptr使用move转移控制权
    buffers_.push_back(std::move(currentBuffer_));

    //备用buffer，减少new次数
    if (nextBuffer_)
    {
      currentBuffer_ = std::move(nextBuffer_);
    }
    else
    {
      //前端写入太快，之前传入后端的buffer还未返回
      currentBuffer_.reset(new Buffer); // muduo：Rarely happens
    }
    currentBuffer_->append(logline, len);
    //唤醒日志线程
    cond_.notify();
  }
}

void AsyncLogging::threadFunc()
{
  //创建日志文件
  LogFile output(basename_, rollSize_);
  Logger::setFlush(std::bind(&LogFile::flush,&output));
  
  
  //进一步减少前端new buffer的次数
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();
  //准备写入文件用的buffer，使用swap与buffer_交换指针，避免等待buffer_直接写入磁盘导致临界区过长
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);
  while (running_)
  {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {
      MutexLockGuard lock(mutex_);
      //前端写入信息量少，没有触发push_back
      if (buffers_.empty())  // unusual usage!
      {
        //cond_.waitForSeconds(flushInterval_);
        cond_.wait();
      }
      //当buffer不为空或者超时，收集前端未满（正在记录）的buffer
      buffers_.push_back(std::move(currentBuffer_));
      //一层缓冲
      currentBuffer_ = std::move(newBuffer1);
      //缩短临界区
      buffersToWrite.swap(buffers_);
      //nextbuffer正在被前端当作currentbuffer使用
      if (!nextBuffer_)
      {
        //二层缓冲
        nextBuffer_ = std::move(newBuffer2);
      }
    }//临界区结束

    assert(!buffersToWrite.empty());

    //消息过多则丢弃（始终保留两块buffer回收使用）
    if (buffersToWrite.size() > 25)
    {
      char buf[256];
      snprintf(buf, sizeof buf, "Dropped log messages %zd larger buffers\n",
               buffersToWrite.size()-2);
      fputs(buf, stderr);
      output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
    }

    for (const auto& buffer : buffersToWrite)
    {
      output.append(buffer->data(), buffer->length());
    }

    //缩短vector以便继续使用，长度为2便于回收
    if (buffersToWrite.size() > 2)
    {
      buffersToWrite.resize(2);
    }

    //回收
    if (!newBuffer1)
    {
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }
    //回收
    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}

