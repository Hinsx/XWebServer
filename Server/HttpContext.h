#ifndef XHTTPCONTEXT_H
#define XHTTPCONTEXT_H

#include"HttpRequest.h"



class Buffer;
//封装http解析过程和解析结果
class HttpContext 
{
 public:
 //请求解析状态，基于状态机解析
  enum HttpRequestParseState
  {
    kExpectRequestLine,
    kExpectHeaders,
    //暂不支持GET之外的请求
    //kExpectBody,
    kGotAll,
  };

  HttpContext()
    : state_(kExpectRequestLine)
  {
  }

  //请求解析，返回成功或失败（若是因字节数不够呢？因为解析状态保存在成员中，在新数据到来后可以直接在原位置继续解析）
  bool parseRequest(Buffer* buf);

  bool gotAll() const
  { return state_ == kGotAll; }

  void reset()
  {
    state_ = kExpectRequestLine;
    HttpRequest dummy;
    request_.swap(dummy);
  }

  const HttpRequest& request() const
  { return request_; }

  HttpRequest& request()
  { return request_; }

 private:
  //解析请求行
  bool processRequestLine(const char* begin, const char* end);

  HttpRequestParseState state_;
  HttpRequest request_;
};



#endif  // XHTTPCONTEXT_H
