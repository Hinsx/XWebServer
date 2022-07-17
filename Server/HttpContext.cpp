
#include "Buffer.h"
#include "HttpContext.h"

#include<algorithm>

bool HttpContext::processRequestLine(const char* begin, const char* end)
{
  bool succeed = false;
  const char* start = begin;
  const char* space = std::find(start, end, ' ');
  //若找到方法名，且方法名合法（GET）
  if (space != end && request_.setMethod(start, space))
  {
    start = space+1;
    space = std::find(start, end, ' ');
    //寻找请求URL
    if (space != end)
    {
      
      const char* question = std::find(start, space, '?');
      //携带额外数据
      if (question != space)
      {
        request_.setPath(start, question);
        request_.setQuery(question, space);
      }
      else
      {
        request_.setPath(start, space);
      }
      start = space+1;
      succeed = (end-start == 8 && std::equal(start, end-1, "HTTP/1."));
      //若协议名正确(HTTP1.X)
      if (succeed)
      {
        if (*(end-1) == '1')
        {
          request_.setVersion(HttpRequest::kHttp11);
        }
        else if (*(end-1) == '0')
        {
          request_.setVersion(HttpRequest::kHttp10);
        }
        else
        {
          succeed = false;
        }
      }
    }
  }
  return succeed;
}

bool HttpContext::parseRequest(Buffer* buf)
{
  bool ok = true;
  bool hasMore = true;
  while (hasMore)
  {
    //当前需要解析请求行
    if (state_ == kExpectRequestLine)
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        ok = processRequestLine(buf->peek(), crlf);
        if (ok)
        {
          //request_.setReceiveTime(receiveTime);
          //只有成功解析请求行，readerIndex才会移动
          buf->retrieveUntil(crlf + 2);
          state_ = kExpectHeaders;
        }
        else
        {
          hasMore = false;
        }
      }
      else
      {
        hasMore = false;
      }
      //如果请求行解析失败，或者没有回车换行，则失败
    }
    //当前需要解析请求头
    else if (state_ == kExpectHeaders)
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        const char* colon = std::find(buf->peek(), crlf, ':');
        if (colon != crlf)
        {
          request_.addHeader(buf->peek(), colon, crlf);
        }
        else
        {
          state_ = kGotAll;
          hasMore = false;
        }
        //只有找到回车换行（找到一个头部字段），readerIndex才会移动
        buf->retrieveUntil(crlf + 2);
      }
      else
      {
        hasMore = false;
      }
    }
    // else if (state_ == kExpectBody)
    // {
    //   //暂不支持GET之外的请求
    // }
  }
  return ok;
}
