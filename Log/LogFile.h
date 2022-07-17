#ifndef XLOGFILE_H 
#define XLOGFILE_H
#include"../Server/Lock.h"
#include <string>
#include<memory>
//日志文件，包含滚动功能，超过记录上限或者超时（一天一周期）则创建新文件
class LogFile{
    public:
    LogFile(const LogFile&)=delete;
    void operator=(const LogFile&)=delete;
    LogFile(const std::string& basename,
          int rollSize,
          int flushInterval = 3,
          int checkEveryN = 1024);
  ~LogFile();

  void append(const char* logline, int len);
  void flush();
  //日志线程启动时，或者写入数据后，进行滚动操作（需要的话）
  bool rollFile();

 private:
  void append_unlocked(const char* logline, int len);

  static std::string getLogFileName(const std::string& basename, time_t* now);

  const std::string basename_;
  const int rollSize_;
  //刷新间隔，固定一段时间后刷新缓冲区
  const int flushInterval_;
  //写入次数达到checkEveryN_值则进行检查，滚动或者刷新缓冲
  const int checkEveryN_;

  int count_;
  
  //只有一个日志线程
  //std::unique_ptr<MutexLock> mutex_;
  time_t startOfPeriod_;
  time_t lastRoll_;
  time_t lastFlush_;
  //日志文件指针
  FILE* file_;
  //已经写入当前日志的字节数
  int writtenBytes_;

  //一天一周期
  const static int kRollPerSeconds_ = 60*60*24;
};

#endif