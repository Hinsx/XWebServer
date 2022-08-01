
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
        request_.setQuery(question+1, space);
        request_.parseQuery();
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
      //首先找到一个请求头的末尾
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
          //get请求直接忽略报文体，解析成功
          if(request_.methodString()=="GET"){
            state_=kGotAll;
            hasMore=false;
          }
          state_ = kExpectBody;
        }
        //只有找到回车换行（找到一个头部字段），readerIndex才会移动
        buf->retrieveUntil(crlf + 2);
      }
      //这个分支一般不会进入，除非只有一个状态行（比如报文只有短短的"GET URL HTTP1.0\r\n")
      else
      {
        hasMore = false;
      }
    }
    //解析报文体,根据content-lenght字段解析报文体
    else if (state_ == kExpectBody)
    {
      //计算包体的长度
      const std::string& lenght_str=request_.getHeader("Content-Length");
      int lenght=0;
      if(!lenght_str.empty())
      {
        lenght=atoi(lenght_str.c_str());
      }
      //当接收的数据达到包体长度
      if(buf->readableBytes()>=lenght)
      {
        request_.setBody(buf->peek(),buf->peek()+lenght);
        buf->retrieve(lenght);
        request_.parseBody();
        hasMore=false;
        state_=kGotAll;
      }
    }
  }
  return ok;
}
