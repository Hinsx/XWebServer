#ifndef XINETADDRESS_H
#define XINETADDRESS_H

#include<arpa/inet.h>
#include <string>
#include<string.h>

// sockaddr_in封装,仅ipv4
class InetAddress
{
public:
  explicit InetAddress(const struct sockaddr_in addr)
      : addr_(addr)
  {
  }
  InetAddress()
  {
    bzero(&addr_, sizeof(addr_));
  }
  //绑定特定端口，所有网卡的地址
  InetAddress(int port)
  {
    bzero(&addr_, sizeof(sockaddr_in));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  //绑定特定地址特定端口
  InetAddress(const char *ip, int port)
  {
    bzero(&addr_, sizeof(sockaddr_in));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr_.sin_addr);
  }

  //获取点分十进制ip
  std::string toIp() const;
  //获取点分十进制ip和端口号
  std::string toIpPort() const;
  //获取端口号
  uint16_t port() const;
  //静态版本

  //获取点分十进制ip
  static std::string addrToIp(sockaddr_in);
  static std::string addrToIp(InetAddress&);
  //获取点分十进制ip和端口号
  static std::string addrToIpPort(sockaddr_in);
  static std::string addrToIpPort(InetAddress&);

  //获取网络字节序表示的地址
  uint32_t ipv4NetEndian() const;
  //网络字节序表示的端口
  uint16_t portNetEndian() const { return addr_.sin_port; }

  sockaddr_in &getAddr()
  {
    return addr_;
  }


private:
  struct sockaddr_in addr_;
};

#endif
