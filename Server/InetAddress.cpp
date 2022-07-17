#include"InetAddress.h"

#include<assert.h>
#include<arpa/inet.h>
#include<string.h>


void _toIp(char* buf, size_t size,const struct sockaddr* addr)
{
    assert(size >= INET_ADDRSTRLEN);
    const sockaddr_in* addr4 = (sockaddr_in*)addr;
    inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));

}

void _toIpPort(char* buf, size_t size,const sockaddr* addr)
{
  _toIp(buf, size, addr);
  size_t end = strlen(buf);
  const sockaddr_in* addr4 =(sockaddr_in*)(addr);
  uint16_t port = be16toh(addr4->sin_port);
  assert(size > end);
  snprintf(buf+end, size-end, ":%u", port);
}

std::string InetAddress::toIpPort() const
{
  char buf[64] = "";
  _toIpPort(buf,sizeof(buf), (sockaddr*)&addr_);
  return buf;
}

std::string InetAddress::toIp() const
{
  char buf[64] = "";
  _toIp(buf, sizeof buf, (sockaddr*)&addr_);
  return buf;
}

std::string InetAddress::addrToIp(sockaddr_in addr){
  char buf[64] = "";
  _toIp(buf, sizeof buf, (sockaddr*)&addr);
  return buf;
}
std::string InetAddress::addrToIp(InetAddress& addr){
  return addrToIp(addr.getAddr());
}

std::string InetAddress::addrToIpPort(sockaddr_in addr)
{
  char buf[64] = "";
  _toIpPort(buf,sizeof(buf), (sockaddr*)&addr);
  return buf;
}
std::string InetAddress::addrToIpPort(InetAddress& addr)
{
  return addrToIpPort(addr.getAddr());
}


uint32_t InetAddress::ipv4NetEndian() const
{
  return addr_.sin_addr.s_addr;
}

uint16_t InetAddress::port() const
{
  return be16toh(portNetEndian());
}


