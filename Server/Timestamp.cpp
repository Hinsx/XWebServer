#include"Timestamp.h"
#include <sys/time.h>

using std::string;

string Timestamp::toFormattedString(bool showMicroseconds)const{
    //不使用time系统调用
    time_t seconds=static_cast<time_t>(microSeconds_/kMicroSecondsPerSecond);
    tm tm_time;
    //将当前时间(秒数）以日历形式存储到tm_time
    gmtime_r(&seconds,&tm_time);

    //存储格式化字符串
    char buf[64]={0};
    if(showMicroseconds){
        //YYYYMMDD hh:mm:ss.uuuuuu
        //获取不足一秒的额外微秒
        int microseconds=static_cast<int>(microSeconds_%kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
             microseconds);
    }
    else{
        //YYYYMMDD hh:mm:ss
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
}
Timestamp Timestamp::now(){
    timeval tv;
    //获取当前时间(秒+微秒),
    gettimeofday(&tv,nullptr);
    long long seconds=tv.tv_sec;
    return Timestamp(seconds*kMicroSecondsPerSecond+tv.tv_usec);
}