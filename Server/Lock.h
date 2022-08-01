#ifndef LOCK_H
#define LOCK_H
#include <pthread.h>
class MutexLockGuard;
class Condition;
//封装互斥锁操作,使用RAII手法.
class MutexLock
{
    friend MutexLockGuard;
    friend Condition;

public:
    MutexLock()
    {

        pthread_mutex_init(&mutex, nullptr);
    }
    ~MutexLock()
    {
        pthread_mutex_destroy(&mutex);
    }
    void lock()
    {
        pthread_mutex_lock(&mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }
    //禁止复制构造和赋值
    MutexLock(const MutexLock &) = delete;
    MutexLock &operator=(const MutexLock &) = delete;

private:
    //用于条件变量
    pthread_mutex_t *getMutex()
    {
        return &mutex;
    }
    pthread_mutex_t mutex;
};

//保证mutex不会忘记解锁
class MutexLockGuard
{
public:
    explicit MutexLockGuard(MutexLock &mutex) : mutex_(mutex)
    {
        mutex_.lock();
    }
    ~MutexLockGuard()
    {
        mutex_.unlock();
    }
    //禁止复制构造和赋值
    MutexLockGuard(const MutexLockGuard &) = delete;
    MutexLockGuard &operator=(const MutexLockGuard &) = delete;

private:
    MutexLock &mutex_;
};
//避免遗漏对象名，创建了临时MutexLockGuard对象后立刻释放，导致没有成功上锁
#define MutexLockGuard(x) static_assert(false, "missing mutex guard var name")

// RAII封装条件变量
class Condition
{
public:
    explicit Condition(MutexLock &mutex) : mutex_(mutex)
    {
        pthread_cond_init(&pcond_, nullptr);
    }
    ~Condition()
    {
        pthread_cond_destroy(&pcond_);
    }
    void wait()
    {
        pthread_cond_wait(&pcond_, mutex_.getMutex());
    }
    void waitForSeconds(double timeout)
    {
        /*
        获取系统启动时到现在经过的秒和微妙，计算到期时间的秒和微秒，设置条件变量使用CLOCK_MONOTONIC
        */
        pthread_condattr_t arr;
        pthread_condattr_setclock(&arr, CLOCK_MONOTONIC);
        timespec target_tm;
        clock_gettime(CLOCK_MONOTONIC, &target_tm);
        const long long kNanoSecondsPerSecond = 1000000000;
        const long long intervalNanoSeconds = static_cast<long long>(timeout * kNanoSecondsPerSecond);
        target_tm.tv_sec += static_cast<time_t>((target_tm.tv_nsec+intervalNanoSeconds) / kNanoSecondsPerSecond);
        target_tm.tv_nsec = static_cast<long>((target_tm.tv_nsec+intervalNanoSeconds) % kNanoSecondsPerSecond);
        pthread_cond_timedwait(&pcond_, mutex_.getMutex(), &target_tm);
    }
    void notify()
    {
        pthread_cond_signal(&pcond_);
    }
    void notifyAll()
    {
        pthread_cond_broadcast(&pcond_);
    }
    //禁止复制构造和赋值
    Condition(const Condition &) = delete;
    Condition &operator=(const Condition &) = delete;

private:
    MutexLock &mutex_;
    pthread_cond_t pcond_;
};
#endif