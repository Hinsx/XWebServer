#ifndef XHTTPRESPONSE_H
#define XHTTPRESPONSE_H



#include <map>
#include<string>

class Buffer;

//http回复报文
class HttpResponse {
 public:
  //响应状态
  enum HttpStatusCode
  {
    kUnknown,
    //正常
    k200Ok = 200,
    //资源转移，返回新URL
    k301MovedPermanently = 301,
    //请求本身格式出错
    k400BadRequest = 400,
    //访问的资源不存在
    k404NotFound = 404,
  };

  explicit HttpResponse(bool close)
    : statusCode_(kUnknown),
      closeConnection_(close)
  {
  }

  void setStatusCode(HttpStatusCode code)
  { statusCode_ = code; }

  void setStatusMessage(const std::string& message)
  { statusMessage_ = message; }

  //http1.0默认关闭长连接
  void setCloseConnection(bool on)
  { closeConnection_ = on; }

  bool closeConnection() const
  { return closeConnection_; }

  void setContentType(const std::string& contentType)
  { addHeader("Content-Type", contentType); }

  void addHeader(const std::string& key, const std::string& value)
  { headers_[key] = value; }

  void setBody(const std::string& body)
  { body_ = body; }

  void appendToBuffer(Buffer* output) const;

 private:
  std::map<std::string, std::string> headers_;
  HttpStatusCode statusCode_;
  // FIXME: add http version
  std::string statusMessage_;
  bool closeConnection_;
  std::string body_;
};


#endif  // XHTTPRESPONSE_H
