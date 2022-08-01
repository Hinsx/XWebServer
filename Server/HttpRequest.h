
#ifndef XHTTPREQUEST_H
#define XHTTPREQUEST_H

#include <map>
#include <assert.h>
#include <stdio.h>
#include <string>
#include <algorithm>
class HttpRequest
{
  typedef std::string::iterator StringIter;

public:
  //协议方法
  enum Method
  {
    kInvalid,
    kGet,
    kPost,
    kHead,
    kPut,
    kDelete
  };
  // http协议版本吗
  enum Version
  {
    kUnknown,
    kHttp10,
    kHttp11
  };

  HttpRequest()
      : method_(kInvalid),
        version_(kUnknown)
  {
  }

  void setVersion(Version v)
  {
    version_ = v;
  }

  Version getVersion() const
  {
    return version_;
  }

  bool setMethod(const char *start, const char *end)
  {
    assert(method_ == kInvalid);
    std::string m(start, end);
    //仅支持get
    if (m == "GET")
    {
      method_ = kGet;
    }
    else if (m == "POST")
    {
      method_ = kPost;
    }
    // else if (m == "HEAD")
    // {
    //   method_ = kHead;
    // }
    // else if (m == "PUT")
    // {
    //   method_ = kPut;
    // }
    // else if (m == "DELETE")
    // {
    //   method_ = kDelete;
    // }
    else
    {
      method_ = kInvalid;
    }
    return method_ != kInvalid;
  }

  Method method() const
  {
    return method_;
  }

  //返回方法名
  const char *methodString() const
  {
    const char *result = "UNKNOWN";
    switch (method_)
    {
    case kGet:
      result = "GET";
      break;
    case kPost:
      result = "POST";
      break;
      //   case kHead:
      //     result = "HEAD";
      //     break;
      //   case kPut:
      //     result = "PUT";
      //     break;
      //   case kDelete:
      //     result = "DELETE";
      //     break;
    default:
      break;
    }
    return result;
  }
  //设置文件请求路径
  void setPath(const char *start, const char *end)
  {
    path_.assign(start, end);
  }
  //设置报文体
  void setBody(const char *start, const char *end)
  {
    body_.assign(start, end);
  }

  const std::string &path() const
  {
    return path_;
  }

  //存储路径'?'之后的请求
  void setQuery(const char *start, const char *end)
  {
    query_.assign(start, end);
  }
  //解析query，拆分key-value对
  void parseQuery()
  {
    if (!query_.empty())
    {
      int readerIndex = 0;
      StringIter end;
      do
      {
        end = std::find(query_.begin() + readerIndex, query_.end(), '&');
        parseKV(query_.begin() + readerIndex, end);
        readerIndex = end - query_.begin() + 1;
      } while (end != query_.end());
    }
  }
  //解析body，拆分key-value对
  void parseBody()
  {
    if (!body_.empty())
    {
      int readerIndex = 0;
      StringIter end;
      do
      {
        end = std::find(body_.begin() + readerIndex, body_.end(), '&');
        parseKV(body_.begin() + readerIndex, end);
        readerIndex = end - body_.begin() + 1;
      } while (end != body_.end());
    }    
  }

  const std::string &query()
  {
    return query_;
  }

  const std::string body()
  {
    return body_;
  }

  // colon指向头部字段:位置
  void addHeader(const char *start, const char *colon, const char *end)
  {
    std::string field(start, colon);
    ++colon;
    //跳过前置空格
    while (colon < end && isspace(*colon))
    {
      ++colon;
    }
    std::string value(colon, end);
    //除去后置空格
    while (!value.empty() && isspace(value[value.size() - 1]))
    {
      value.resize(value.size() - 1);
    }
    headers_[field] = value;
  }


  //获取某头部字段的value,需要const后置声明以允许const this指针调用
  std::string getHeader(const std::string &field) const
  {
    std::string result;
    std::map<std::string, std::string>::const_iterator it = headers_.find(field);
    if (it != headers_.end())
    {
      result = it->second;
    }
    return result;
  }



  //根据key获取value值,参数可能来自url，也可能来自请求体
  std::string getValueByKey(const std::string &key) const
  {
    std::string result;
    std::map<std::string, std::string>::const_iterator it = key_values_.find(key);
    if (it != key_values_.end())
    {
      result = it->second;
    }
    return result;
  }

  const std::map<std::string, std::string> &headers()
  {
    return headers_;
  }

  //快速重置,用于HttpContext reset函数
  void swap(HttpRequest &that)
  {
    std::swap(method_, that.method_);
    std::swap(version_, that.version_);
    path_.swap(that.path_);
    query_.swap(that.query_);
    headers_.swap(that.headers_);
  }

private:

  //拆分key-value值
  void parseKV(StringIter begin, StringIter end)
  {
    auto equal = std::find(begin, end, '=');
    if (equal != end)
    {
      std::string key(begin, equal);
      std::string value(equal + 1, end);
      key_values_[key] = value;
    }
  }


  Method method_;
  Version version_;
  std::string path_;
  std::string query_;
  std::string body_;
  //存储头部字段和对应的值
  std::map<std::string, std::string> headers_;
  //存储get请求携带key-value值
  std::map<std::string, std::string> querykvs_;
  //存储请求体携带的key-value值
  std::map<std::string, std::string> bodykvs_;
  //存储头部或请求体携带的key-value值
  std::map<std::string, std::string> key_values_;
};

#endif
