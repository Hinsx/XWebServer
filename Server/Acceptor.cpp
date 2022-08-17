#include "Acceptor.h"
#include"EventLoop.h"
#include "../Log/Logger.h"
#include <iostream>
#include <assert.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include"Channel.h"

Acceptor::Acceptor(EventLoop* loop,const InetAddress& localAddr) :
loop_(loop),
addr_(localAddr),
listen_fd(),//创建非阻塞监听fd
channel_(loop,listen_fd.fd()),
idleFd_(open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    int flag=1;
    setsockopt(listen_fd.fd(), SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    listen_fd.sock_bind(addr_.getAddr());
    channel_.setReadCallBack(std::bind(&Acceptor::accept,this));
    channel_.enableReading();
    LOG_DEBUG << "Acceptor init success.";
}


void Acceptor::listen(int size)
{
    listen_fd.sock_listen(size);
}

void Acceptor::accept(){
    int connfd=1;
    while(connfd>0){
        InetAddress peerAddr;
        connfd=listen_fd.sock_accept(peerAddr.getAddr());
        LOG_TRACE<<peerAddr.toIpPort()<<"\n";
        if(connfd<0){
            LOG_SYSERR << "-------Accept failed.----------";
            /*
            当lfd处理poller监听中，当新连接到来时lfd可读，于是poller返回
            若此时文件描述符已经耗尽，则无法从全连接队列中取出新连接（无法为其开辟socket缓冲区）
            1.如果不做任何处理，因为lfd仍然可读（仍然有连接），下一次poller立即返回，如此反复，造成busyloop。若是ET模式，那么
            2.如果提前占一个文件描述符idle，当上述情况发生时，可以先将idle关闭，空出一个描述符，然后accept后立刻关闭
            疑问：为何使用固定的/dev/null，任意socket是否可行？
            
            */

            //文件描述符耗尽
            if(errno==EMFILE){
                LOG_DEBUG << "Throwing new connection...";
                close(idleFd_);
                idleFd_=listen_fd.sock_accept(peerAddr.getAddr());
                close(idleFd_);
                idleFd_=open("/dev/null", O_RDONLY | O_CLOEXEC);
            }
        }
        
        else{
            LOG_TRACE << "Acceptor get new connection.";
            newConnectionCallback_(connfd,peerAddr);
        }
    }
    
}

Acceptor::~Acceptor()
{
  channel_.disableAll();
  channel_.remove();
  close(idleFd_);
}