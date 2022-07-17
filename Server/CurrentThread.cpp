

#include "CurrentThread.h"

namespace CurrentThread
{
//存储线程唯一id（pthread_t在不同进程可能相同）
__thread int t_cachedTid = 0;
//存储id的字符串形式，用于日志输出
__thread char t_tidString[32];
//字符串形式id的长度
__thread int t_tidStringLength = 6;
//线程的自定义名称
__thread const char* t_threadName = "unknown";

}  // namespace CurrentThread

