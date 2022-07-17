#include"Logger.h"
#include"../Server/CurrentThread.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include<assert.h>
#include<time.h>

#include <sstream>

//__thread修饰，所有线程各自一份拷贝
//存储errno信息
__thread char t_errnobuf[512];
//__thread char t_time[64];
//__thread time_t t_lastSecond;

const char* strerror_tl(int savedErrno)
{
  return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

//默认日志等级为INFO
Logger::LogLevel initLogLevel()
{
  return static_cast<Logger::LogLevel>(2);
}

Logger::LogLevel g_logLevel = initLogLevel();

//等级字符串，长度固定为6
const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};


//使用此类检查字符串长度，避免越界
class T
{
 public:
  T(const char* str, size_t len)
    :str_(str),
     len_(len)
  {
    assert(strlen(str) == len_);
  }

  const char* str_;
  const size_t len_;
};

inline LogStream& operator<<(LogStream& s, T v)
{
  s.append(v.str_, v.len_);
  return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
{
  s.append(v.data_, v.size_);
  return s;
}



//日志信息默认输出到标准输出流（stdout），fwrite效率高
void defaultOutput(const char* msg, int len)
{
  size_t n = fwrite(msg, 1, len, stdout);
}
//默认刷新的缓冲区为stdout的缓冲区
void defaultFlush()
{
  fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;


void Logger::Impl::inputCurrentTime(){
	char timebuf[32];
  time_t now= time(NULL);
  struct tm tm;
  gmtime_r(&now, &tm); // FIXME: localtime_r ?
  strftime(timebuf, sizeof timebuf, "%Y-%m-%d %H:%M:%S ", &tm);
  stream_<<timebuf;
}
Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
  : stream_(),
    level_(level),
    line_(line),
    basename_(file)
{
  inputCurrentTime();
  stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
  stream_ << T(LogLevelName[level_], 6);
  if (savedErrno != 0)
  {
    stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
  }
}

void Logger::Impl::finish()
{
  stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger(SourceFile file, int line)
  : impl_(INFO, 0, file, line)
{
}

//DEBUG等级
Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
  : impl_(level, 0, file, line)
{
  impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level)
  : impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, bool toAbort)
  : impl_(toAbort?FATAL:ERROR, errno, file, line)
{
}

Logger::~Logger()
{
  impl_.finish();
  const LogStream::Buffer& buf(stream().buffer());
  g_output(buf.data(), buf.length());
  //严重错误，将缓冲区信息刷出，停止系统
  if (impl_.level_ == FATAL)
  {
    g_flush();
    abort();
  }
}

void Logger::setLogLevel(int n)
{
  assert(n>=0&&n<Logger::NUM_LOG_LEVELS);
  g_logLevel =static_cast<Logger::LogLevel>(n);
}

void Logger::setOutput(OutputFunc out)
{
  g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
  g_flush = flush;
}

