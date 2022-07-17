#ifndef XLOGGING_H
#define XLOGGING_H

#include <string.h> // memcpy
#include <string>
using std::string;

namespace detail
{
  //定义两种buffer长度
  const int kSmallBuffer = 4000;
  const int kLargeBuffer = 4000 * 1000;

  //小型缓冲区，临时存储一段格式化的信息
  template <int SIZE>
  class FixedBuffer
  {
  public:
    FixedBuffer()
        : cur_(data_)
    {
    }
    //将字符串添加到buffer
    void append(const char *buf, size_t len)
    {
      //有足够的空间
      if (static_cast<size_t>(avail()) > len)
      {
        memcpy(cur_, buf, len);
        cur_ += len;
      }
    }

    //返回缓冲区
    const char *data() const { return data_; }

    //返回已使用的缓冲区长度
    int length() const { return static_cast<int>(cur_ - data_); }

    char *current() { return cur_; }
    //返回剩余空缓冲区长度
    int avail() const { return static_cast<int>(end() - cur_); }
    void add(size_t len) { cur_ += len; }

    //重置缓冲区
    void reset() { cur_ = data_; }
    void bzero() { memset(data_, 0, sizeof(data_)); }

    FixedBuffer(const FixedBuffer &) = delete;
    FixedBuffer &operator=(const FixedBuffer &) = delete;

  private:
    const char *end() const { return data_ + sizeof data_; }

    // buffer
    char data_[SIZE];
    //指向buffer空闲区开头
    char *cur_;
  };

} // namespace detail

//重定义<<,使用<<输出消息(支持整数/字符串/字符),存储到buffer中
class LogStream
{
  typedef LogStream self;

public:
  typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

  self &operator<<(bool v)
  {
    buffer_.append(v ? "1" : "0", 1);
    return *this;
  }

  self &operator<<(short);
  self &operator<<(unsigned short);
  self &operator<<(int);
  self &operator<<(unsigned int);
  self &operator<<(long);
  self &operator<<(unsigned long);
  self &operator<<(long long);
  self &operator<<(unsigned long long);

  self &operator<<(char v)
  {
    buffer_.append(&v, 1);
    return *this;
  }
  template <typename T>
  self &operator<<(const T *ptr)
  {
    return operator<<((const void *)ptr);
  }
  self &operator<<(const void *ptr);

  self &operator<<(const char *str)
  {
    if (str)
    {
      buffer_.append(str, strlen(str));
    }
    else
    {
      buffer_.append("(null)", 6);
    }
    return *this;
  }

 

  self &operator<<(const string &v)
  {
    buffer_.append(v.c_str(), v.size());
    return *this;
  }
  //字符串输入最终调用append
  void append(const char *data, int len) { buffer_.append(data, len); }
  const Buffer &buffer() const { return buffer_; }
  void resetBuffer() { buffer_.reset(); }

private:
  template <typename T>
  //将整数格式化为字符串
  void formatInteger(T);

  Buffer buffer_;

  // muduo网络库设置的限制，数字填入缓冲区需要保证至少48字节空闲区可用
  static const int kMaxNumericSize = 48;
};

#endif
