#ifndef XLOGGER_H
#define XLOGGER_H

#include"LogStream.h"
#include"../Server/Timestamp.h"
#include<functional>
class TimeZone;



/*
封装了日志等级、日志输入输出
内置impl类，通过宏定义创建logger临时对象，
impl存储信息，logger析构时输出到后端/标准输出流
*/
class Logger
{
 public:
 //日志过滤等级
  enum LogLevel
  {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    //在.cpp中创建对应的字符串数组(用于日志输出），根据该值设置数组大小，后期若添加等级只需增加新枚举值,修改.cpp的字符串数组
    NUM_LOG_LEVELS,
  };

  //存储调用LOG时的源文件名称
  class SourceFile
  {
   public:
    template<int N>
    SourceFile(const char (&arr)[N])
      : data_(arr),
        size_(N-1)
    {}

    // explicit SourceFile(const char* filename)
    //   : data_(filename)
    // {
    //   size_ = static_cast<int>(strlen(data_));
    // }
    //存储文件名
    const char* data_;
    //文件名长度
    int size_;
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream& stream() { return impl_.stream_; }

  //查询日志过滤等级
  static LogLevel logLevel();
  //设置日志过滤等级
  static void setLogLevel(int level);

  //日志输出的接口，可以对接到后端输入磁盘，默认输出到stdout
  typedef std::function<void(const char* msg, int len)>OutputFunc;
  //刷新缓冲区（默认输出到stdout)，发生在系统故障调用abort前
  typedef std::function<void()>FlushFunc;
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);

 private:

/*
单条信息结构，信息存储于stream_成员,基本信息包括：
[时间（待实现），当前线程tid，信息等级，源文件名，行号]
根据等级可能还包括额外的基本信息，包括：
1.调用的函数名称（DEBUG级），
2.当前发生的错误及其描述（FATAL级，并且是LOG_SYSFATAL调用，意味着系统调用自身出现问题）
*/
class Impl
{
 public:
  typedef Logger::LogLevel LogLevel;
  Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
  void inputCurrentTime();
  //void formatTime();
  //在logger析构时调用此函数，将文件名和行号写入stream_
  void finish();

  Timestamp time_;
  LogStream stream_;
  LogLevel level_;
  int line_;
  //文件名，来自__FILE__系统宏定义
  SourceFile basename_;
};

  Impl impl_;

};

//全局日志等级，配合宏定义过滤日志信息,读取配置文件进行设置（012）
extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel()
{
  return g_logLevel;
}

#define LOG_TRACE if (Logger::logLevel() <= Logger::TRACE) \
  Logger(__FILE__, __LINE__, Logger::TRACE, __func__).stream()

#define LOG_DEBUG if (Logger::logLevel() <= Logger::DEBUG) \
  Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).stream()

#define LOG_INFO if (Logger::logLevel() <= Logger::INFO) \
  Logger(__FILE__, __LINE__).stream()

#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).stream()

#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()

#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()
#define LOG_SYSERR Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Logger(__FILE__, __LINE__, true).stream()

//记录错误信息到指定缓冲区
const char* strerror_tl(int savedErrno);

#endif