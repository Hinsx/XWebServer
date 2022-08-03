#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

const char Buffer::kCRLF[] = "\r\n";

const int Buffer::kPreBytes;
const int Buffer::kInitialSize;

ssize_t Buffer::readFd(int fd, int *savedErrno)
{
  char extrabuf[65536];
  iovec vec[2];
  const size_t writable = writableBytes();
  //若buffer不足以读取所有数据，可以先将额外数据暂存，不用调用while
  vec[0].iov_base = begin() + writerIndex_;
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

size_t SendMsg::writeToSocket(int fd)
{
  int wrote = 0;
  //首先发送buffer的数据
  if (buffer_.readableBytes() > 0)
  {
    wrote = write(fd, buffer_.peek(), buffer_.readableBytes());
    //如果是0，对端（读）关闭，此时第一次写入返回0，直接认为写入0字节，等待下一次写入触发epipe
    if (wrote >= 0)
    {
      buffer_.retrieve(wrote);
    }
    //发生错误
    if (wrote < 0)
    {
      return -1;
    }
  }
  // buffer空了就发送文件，可能此时缓冲区刚好已满
  if (buffer_.readableBytes() == 0)
  {
    //如果不需要发送文件（有些message不需要发送文件）
    if(file_.faileDone())
    {
      done_=true;
      return 0;
    }
    int filefd = open(file_.filename().c_str(), O_RDONLY | O_NONBLOCK, "rb");
    //零拷贝
    wrote = sendfile(fd, filefd, file_.nworte(), file_.fileSize());
    // if (wrote >= 0)
    // {
    //   file_.retrieve(wrote);
    // }
    //记得关闭文件描述符
    close(filefd);
    if (wrote < 0)
    {
      return -1;
    }
    if (file_.faileDone())
    {
      done_ = true;
    }
  }
  return 0;
}