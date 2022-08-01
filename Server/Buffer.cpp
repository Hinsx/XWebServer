#include"Buffer.h"

#include <errno.h>
#include <sys/uio.h>

const char Buffer::kCRLF[]="\r\n";


const int Buffer::kPreBytes;
const int Buffer::kInitialSize;

ssize_t Buffer::readFd(int fd, int* savedErrno)
{
  char extrabuf[65536];
  iovec vec[2];
  const size_t writable = writableBytes();
  //若buffer不足以读取所有数据，可以先将额外数据暂存，不用调用while
  vec[0].iov_base = begin()+writerIndex_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
  //分散读
  const ssize_t n = readv(fd, vec, iovcnt);
  
  //无法读取（不会是EAGAIN错误，因为提示有数据可读了）
  if (n < 0)
  {
    *savedErrno = errno;
  }
  else if (static_cast<size_t>(n) <= writable)
  {
    writerIndex_ += n;
  }
  else
  {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  return n;
}