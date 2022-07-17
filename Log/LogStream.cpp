#include"LogStream.h"

#include <algorithm>
// #include <limits>
// #include <type_traits>
// #include <assert.h>
// #include <string.h>
// #include <stdint.h>
// #include <stdio.h>

//#include<cstring>
// #include <inttypes.h>

namespace detail
{

const char digitsHex[] = "0123456789ABCDEF";
const char digits[] = "9876543210123456789";
//整数转字符串时，整数为负数，向左偏移,十进制转十六进制同理
const char* zero = digits + 9;

// muduo：Efficient Integer to String Conversions, by Matthew Wilson.
template<typename T>
size_t convert(char buf[], T value)
{
  T i = value;
  char* p = buf;

  do
  {
    int lsd = static_cast<int>(i % 10);
    i /= 10;
    *p++ = zero[lsd];
  } while (i != 0);

  if (value < 0)
  {
    *p++ = '-';
  }
  *p = '\0';
  std::reverse(buf, p);

  
  return p - buf;
}

size_t convertHex(char buf[], uintptr_t value)
{
  uintptr_t i = value;
  char* p = buf;

  do
  {
    int lsd = static_cast<int>(i % 16);
    i /= 16;
    *p++ = digitsHex[lsd];
  } while (i != 0);

  *p = '\0';
  std::reverse(buf, p);
  return p - buf;
}
// //主动实例化
// template class FixedBuffer<kSmallBuffer>;
// template class FixedBuffer<kLargeBuffer>;

}

template<typename T>
void LogStream::formatInteger(T v)
{
  //保证有剩余空间记录源文件和行号
  if (buffer_.avail() >= kMaxNumericSize)
  {
    size_t len = detail::convert(buffer_.current(), v);
    buffer_.add(len);
  }
}

LogStream& LogStream::operator<<(short v)
{
  *this << static_cast<int>(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
  *this << static_cast<unsigned int>(v);
  return *this;
}

LogStream& LogStream::operator<<(int v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(long long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
  formatInteger(v);
  return *this;
}
//将地址值以十六进制打印
LogStream& LogStream::operator<<(const void* ptr){
  //强制转型
  uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
  if (buffer_.avail() >= kMaxNumericSize)
  {
    char* buf = buffer_.current();
    buf[0] = '0';
    buf[1] = 'x';
    size_t len = detail::convertHex(buf+2, v);
    buffer_.add(len+2);
  }
  return *this;
}
