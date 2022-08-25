#ifndef XHTTPSERVER_H
#define XHTTPSERVER_H
#include <vector>
#include <functional>
#include <arpa/inet.h>
#include <string>
#include <memory>
#include <map>
#include<set>
#include<queue>
//#include<boost/circular_buffer.hpp>

#include"Lock.h"
#include "InetAddress.h"
#include"../Log/Logger.h"
#include"HttpConnection.h"
class Channel;
class Acceptor;
class HttpConnection;
class Threadpool;
/*
服务器整体框架：Reactor模式，主线程+io线程池(主从reactor，若线程池数量为0，则主reactor所在线程来管理连接，即单线程reactor)
主线程负责监听连接，创建连接，将新连接交由线程池中某一线程处理，线程选择算法使用时间片的方法。
线程池：对负责的连接进行监听，调用相应事件回调，负责数据读写，处理逻辑如Http信息解析，根据结果返回相应信息

1.不用担心一个socket（http连接）会同时被两个线程操作
2.LT模式/poll造成的busyloop通过及时取消读事件解决
3.通过queenInloop，主线程通知其他io线程新连接的到来

*/
class EventLoop;
class HttpServer
{
    
    typedef std::shared_ptr<HttpConnection> HttpConnectionPtr;
    typedef std::weak_ptr<HttpConnection> WeakHttpConnectionPtr;
    typedef std::map<std::string, HttpConnectionPtr> HttpConnectionMap;
    //时间轮中的元素，拥有连接的弱指针当被析构时会尝试提升连接弱指针，若成功则执行shutdown
    //因为时间轮中使用shareptr管理entry，可能同时存在多个entry，当最后一个entry被踢出后则执行析构
    struct Entry
    {
        explicit Entry(const WeakHttpConnectionPtr &weakConn)
            : weakConn_(weakConn)
        {
        }
        ~Entry()
        {
            LOG_TRACE<<"~Entry() connection timeout.";
            //尝试升级，若失败则说明连接在超时前已析构
            HttpConnectionPtr conn = weakConn_.lock();
            if (conn)
            {
                conn->shutdown();
            }
        }
        WeakHttpConnectionPtr weakConn_;
    };
    typedef std::shared_ptr<Entry>EntryPtr;
    typedef std::weak_ptr<Entry>WeakEntryPtr;
    typedef std::set<EntryPtr>Bucket;
    //typedef boost::circular_buffer<Bucket>WeakConnectionList;
    //typedef std::queue<Bucket>WeakConnectionList;
    struct CircularBuffer{
        std::deque<Bucket>buffer;
        int capacity;
        void setCapacity(int c){capacity=c<0?1:c;}
        void push_back(const Bucket& bucket){
            buffer.push_back(bucket);
            if(buffer.size()>capacity){
                buffer.pop_front();
            }
        }
        Bucket& back(){return buffer.back();}
    };
public:
    //初始化服务器,默认使用poll模式进行监听
    HttpServer(EventLoop *loop, std::string name, const char* ip,int port,int idleSeconds,int maxConnectionNums);
    ~HttpServer();
    //服务器启动
    void start();
    //更新监听的文件描述符
    void update(Channel *channel);
    //连接主动关闭后回调此函数
    void removeConnection(const HttpConnectionPtr &conn);
    // erase连接需要保证临界区（map）的安全
    void removeConnectionInLoop(const HttpConnectionPtr &conn);

    void newConnnectionCallback(int connfd, InetAddress peerAddr);
    //连接需要打开文件时，首先查看是否以及打开，减少open调用
    int fileOpenCallback(std::string& filename);

    /*
    定时器超时回调，新增bucket到尾部，若到达长度上限则弹出头部元素，
    造成entryptr析构,此举又会导致内存堆中entry析构（若计数为0），调用连接shutdown
    */
    void onTimer();

    //为新连接创建一个entry
    void onConnection(const HttpConnectionPtr& conn);
    void onConnectionInLoop(const HttpConnectionPtr& conn);

    //连接重新计时
    void onMessage(const HttpConnectionPtr& conn);
    void onMessageInLoop(const HttpConnectionPtr& conn);

private:

    EventLoop *loop_;
    //服务器名称
    std::string name_;
    //服务器监听地址
    InetAddress localAddr_;
    std::string ipPort_;
    //接收新连接，封装listen等操作
    std::unique_ptr<Acceptor> acceptor;
    //线程池,采用RR算法选择新线程
    std::unique_ptr<Threadpool> pool_;
    //时间轮
    CircularBuffer connectionBuckets_;
    //空闲连接停留时间
    int idleSeconds_;
    //连接id，标识连接用于映射
    int nextConnId_;
    //最大连接数量
    int maxConnectionNums_;
    //存储文件名及其对应的文件描述符
    std::map<std::string,int>files_;
    //互斥修改map
    RWLock lock;
    //智能指针处理连接
    HttpConnectionMap connections_;
};

#endif