#ifndef XTIMESTAMP_H
#define XTIMESTAMP_H
#include <string>
//对时间进行封装，以微秒表示
class Timestamp
{
public:
    static const int kMicroSecondsPerSecond = 1000000;
    //获取当前时间,使用gettimeofday，在64位系统是用户态实现
    static Timestamp now();
    
    static Timestamp invalid()
    {
        return Timestamp();
    }
    explicit Timestamp(long long microSeconds) : microSeconds_(microSeconds)
    {
    }
    Timestamp() : microSeconds_(0)
    {
    }
    //输出时间格式化表示，可选择是否到微妙精度
    std::string toFormattedString(bool showMicroseconds = true) const;

    long long microSeconds() const { return microSeconds_; }
    time_t seconds() const
    {
        return static_cast<time_t>(microSeconds_ / kMicroSecondsPerSecond);
    }
    bool valid() const { return microSeconds_ > 0; }

private:
    long long microSeconds_;
};
inline bool operator<(Timestamp l, Timestamp r)
{
    return l.microSeconds() < r.microSeconds();
}
inline bool operator__(Timestamp l, Timestamp r)
{
    return l.microSeconds() == r.microSeconds();
}
//根据已有时间构造未来时间
inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    long long mircoseconds = static_cast<long long>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSeconds() + mircoseconds);
}
#endif