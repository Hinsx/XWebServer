#ifndef XHTTPSERVER_H
#define XHTTPSERVER_H
#include <vector>
#include<functional>
#include <arpa/inet.h>
#include<string>
#include<memory>
#include<map>

#include"InetAddress.h"
class Channel;
class Acceptor;
class HttpConnection;
class Threadpool;
/*
服务器整体框架：Reactor模式，主线程+线程池
主线程负责监听连接，创建连接，将新连接交由线程池中某一线程处理，线程选择算法使用时间片的方法。
线程池：对负责的连接进行监听，调用相应事件回调，负责数据读写，处理逻辑如Http信息解析，根据结果返回相应信息

1.不用担心一个socket（http连接）会同时被两个线程操作
2.LT模式/poll造成的busyloop通过及时取消读事件解决
3.通过queenInloop，主线程通知其他io线程新连接的到来

*/
class EventLoop;
class HttpServer
{
    typedef std::shared_ptr<HttpConnection>HttpConnectionPtr;
    typedef std::map<std::string,HttpConnectionPtr>HttpConnectionMap;

public:
    //初始化服务器,默认使用poll模式进行监听
    HttpServer(EventLoop* loop,std::string name,bool epoll=true);
    ~HttpServer();
    //服务器启动
    void start();
    //更新监听的文件描述符
    void update(Channel* channel);
    //连接主动关闭后回调此函数
    void removeConnection(const HttpConnectionPtr& conn);

    void newConnnectionCallback(int connfd,InetAddress peerAddr);

private:


    EventLoop* loop_;
    //服务器名称
    std::string name_;
    //服务器监听地址
    InetAddress localAddr_;
    std::string ipPort_;
    //接收新连接，封装listen等操作
    std::unique_ptr<Acceptor>acceptor;
    //线程池,采用RR算法选择新线程
    std::unique_ptr<Threadpool>pool_;
    //连接id，标识连接用于映射
    int nextConnId_;
    //IO复用模式
    bool epoll_=false;

    //智能指针处理连接
    HttpConnectionMap connections_;

};

#endif