#ifndef XCURRENTTHREAD_H
#define XCURRENTTHREAD_H

namespace CurrentThread
{
  //__thread关键字，每个线程拥有独立一份
  extern __thread int t_cachedTid;
  extern __thread char t_tidString[32];
  extern __thread int t_tidStringLength;
  extern __thread const char* t_threadName;
  void cacheTid();

  inline int tid()
  {
    //muduo：分支预测，t_cachedTid==0为假的可能性更大（只有初始化时为假）
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
      cacheTid();
    }
    return t_cachedTid;
  }

  //输出线程tid的字符串形式，用于日志输出
  inline const char* tidString() // for logging
  {
    return t_tidString;
  }
  //输出线程tid的长度，用于日志输出
  inline int tidStringLength() // for logging
  {
    return t_tidStringLength;
  }
  //输出线程名称
  inline const char* name()
  {
    return t_threadName;
  }

  bool isMainThread();

} 


#endif  
