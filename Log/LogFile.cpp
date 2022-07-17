#include "LogFile.h"
#include"Logger.h"
#include <assert.h>
#include <string.h>

LogFile::LogFile(const std::string &basename,
                 int rollSize,
                 int flushInterval,
                 int checkEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      count_(0),
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0),
      writtenBytes_(0)
{
    //日志文件创建的路径只允许在当前目录下
    assert(basename.find('/') == std::string::npos);
    rollFile();
}

LogFile::~LogFile(){
    fclose(file_);
}

void LogFile::append(const char *logline, int len)
{
  size_t written = 0;

  while (written != len)
  {
    //剩余需要写入的字节数量
    size_t remain = len - written;
    size_t n =fwrite_unlocked(logline + written, 1, remain, file_);
    //没有一次性写入指定数量的字节
    if (n != remain)
    {
      int err = ferror(file_);
      //发生错误导致的
      if (err)
      {
        fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
        break;
      }
    }
    written += n;
  }

  writtenBytes_ += written;

    //超过单个日志的写入上限
    if ( writtenBytes_> rollSize_)
    {
        rollFile();
    }
    else
    {
        //写入次数达到上限，进行检查
        ++count_;
        if (count_ >= checkEveryN_)
        {
            count_ = 0;
            time_t now = ::time(NULL);
            time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
            //周期性滚动日志
            if (thisPeriod_ != startOfPeriod_)
            {
                rollFile();
            }
            //定期刷新
            else if (now - lastFlush_ > flushInterval_)
            {
                lastFlush_ = now;
                flush();
            }
        }
    }
}

void LogFile::flush()
{
   fflush(file_);
}


bool LogFile::rollFile()
{
    time_t now = 0;
    std::string filename = getLogFileName(basename_, &now);
    //当前时间所处的周期
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    //距离上一次滚动只差不到一秒，放弃滚动
    if (now > lastRoll_)
    {
        writtenBytes_=0;
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        file_=fopen(filename.c_str(), "a");
        return true;
    }
    return false;
}

//日志文件名格式：basename(用户输入).YYMMDD-HHMMSS.log
std::string LogFile::getLogFileName(const std::string &basename, time_t *now)
{
    std::string filename;
    filename.reserve(basename.size() + 32);
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    gmtime_r(now, &tm);
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S", &tm);
    filename += timebuf;

    filename += ".log";

    return filename;
}