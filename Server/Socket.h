#ifndef SOCKET_H
#define SOCKET_H
#include <arpa/inet.h>
#include<unistd.h>
//封装TCP socket基本操作，包括创建socket，bind,listen,设置socket选项等
class Socket{

    public:
    static void socketSetNONBLOCK(int fd,bool nonblock);
    //创建一个文件描述符,默认非阻塞
    Socket(bool block=false);
    //绑定特定的地址端口
    void sock_bind(sockaddr_in& address);

    void sock_listen(int size=5);

    //接收新连接，返回fd
    int sock_accept(sockaddr_in& address);

    //关闭socket
    void sock_close(){close(sockfd);};
    //允许地址复用
    void setReuseAddr(bool flag);
    //设置阻塞或者非阻塞
    void nonblock(bool nonblock);
    //获取文件描述符
    int fd(){return sockfd;}
    //析构,关闭socket
    ~Socket(){
        close(sockfd);
    }
    private:
    int sockfd;
};

#endif