#include <iostream>
#include <assert.h>
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>
#include <iostream>
#include "Socket.h"
#include"../Log/Logger.h"

void Socket::socketSetNONBLOCK(int fd,bool nonblock){
    if (nonblock)
    {
        int old_opt = fcntl(fd, F_GETFL);
        int new_opt = old_opt | O_NONBLOCK;
        fcntl(fd, F_SETFL, new_opt);
    }
    else{
        int old_opt = fcntl(fd, F_GETFL);
        int new_opt = old_opt & (~O_NONBLOCK);
        fcntl(fd, F_SETFL, new_opt);
    }
}

Socket::Socket(bool block)
{
    if(!block){
        sockfd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);
    }
    else{
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
    }
    LOG_TRACE<<"Create new socket = "<<sockfd;
}
void Socket::nonblock(bool flag)
{
    if (flag)
    {
        int old_opt = fcntl(sockfd, F_GETFL);
        int new_opt = old_opt | O_NONBLOCK;
        fcntl(sockfd, F_SETFL, new_opt);
    }
    else{
        int old_opt = fcntl(sockfd, F_GETFL);
        int new_opt = old_opt & (~O_NONBLOCK);
        fcntl(sockfd, F_SETFL, new_opt);
    }
}
void Socket::sock_bind(sockaddr_in &address)
{
    int ret = bind(sockfd, (sockaddr *)&address, sizeof(sockaddr_in));
    assert(ret != -1);
}
void Socket::sock_listen(int size)
{
    int ret = listen(sockfd, size);
    assert(ret != -1);
}

int Socket::sock_accept(sockaddr_in &address)
{
    socklen_t len=sizeof(address);
    int connfd = accept4(sockfd, (sockaddr *)&address, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    // char client_ip[30];
    // inet_ntop(AF_INET, &address.sin_addr, client_ip, len);
    // LOG_TRACE<<"------"<<client_ip<<"--------.";
    return connfd;
}