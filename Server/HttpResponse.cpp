#include "HttpResponse.h"
#include "Buffer.h"
#include"../Log/Logger.h"
#include <stdio.h>
#include<string>

void HttpResponse::appendToBuffer(Buffer* output) const
{
  char buf[32];
  snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
  output->append(buf);
  output->append(statusMessage_);
  output->append("\r\n");
  if (closeConnection_)
  {
    output->append("Connection: close\r\n");
  }
  else
  {
    //静态页面，直接根据body计算出大小
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
    output->append(buf);
    output->append("Connection: keep-alive\r\n");
  }

  for (const auto& header : headers_)
  {
    output->append(header.first);
    output->append(": ");
    output->append(header.second);
    output->append("\r\n");
  }

  output->append("\r\n");
  output->append(body_);

  //std::string msg;msg.assign(output->peek(),output->readableBytes());
  //LOG_TRACE<<"The http response is:\n"<<msg;
}

void HttpResponse::appendToBuffer_(SendMsg* msg) const
{
  Buffer* output=msg->getBuffer();
  char buf[32];
  snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
  output->append(buf);
  output->append(statusMessage_);
  output->append("\r\n");
  if (closeConnection_)
  {
    output->append("Connection: close\r\n");
  }
  else
  {
    //静态页面，直接根据body计算出大小
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size()+filesize_);
    output->append(buf);
    output->append("Connection: keep-alive\r\n");
  }

  for (const auto& header : headers_)
  {
    output->append(header.first);
    output->append(": ");
    output->append(header.second);
    output->append("\r\n");
  }

  output->append("\r\n");
  output->append(body_);

  //std::string msg;msg.assign(output->peek(),output->readableBytes());
  //LOG_TRACE<<"The http response is:\n"<<msg;
}
