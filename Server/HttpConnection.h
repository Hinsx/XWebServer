#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include "InetAddress.h"
#include "Buffer.h"
#include"HttpContext.h"

#include <string>
#include <memory>
#include<functional>
#include<boost/any.hpp>
#include<list>

class Socket;
class Channel;
class EventLoop;
// http连接，读写数据，解析请求，发起数据库请求，读写数据库等
//因为连接由shared_ptr管理（在主线程）继承enable_shared_from_this<>类，允许shared_ptr指向this指针（如果直接shared_ptr<this>则会造成两个非共享的shared_ptr指向同一对象）
class HttpConnection: public std::enable_shared_from_this<HttpConnection>
{
    
    typedef std::shared_ptr<HttpConnection> HttpConnectionPtr;
    typedef std::function<void (const HttpConnection&)> WriteCompleteCallback;
    typedef std::function<void (const HttpConnectionPtr&)> CloseCallback;
    typedef std::function<void (const HttpConnectionPtr&)> ConnectionCallback;
    typedef std::function<void (const HttpConnectionPtr&)> MessageCallback;
    
public:
typedef std::function<int(std::string&)> FileOpenCallback;
    HttpConnection(EventLoop *loop,
                   const std::string &name,
                   int sockfd,
                   const InetAddress &localAddr,
                   const InetAddress &peerAddr);
    ~HttpConnection();
    
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }
    //连接创建后执行的第一个函数，由主线程向io线程添加，在io线程中执行
    void connectEstablished();

    //server erase连接后调用
    void connectDestroyed();

    void send(const char* message){
        send(message,strlen(message));
    }
    void send(const char *message, size_t len);
    //void send(Buffer *message);
    void send(SendMsg* message);

    void shutdown();

    void setCloseCallback(const CloseCallback& cb){
        closeCallback_=cb;
    }
    void setConnectionCallback(const ConnectionCallback& cb){
        connectionCallback_=cb;
    }
    void setMeesageCallback(const MessageCallback& cb){
        messageCallback_=cb;
    }
    static void setFileopenCallback(const FileOpenCallback& cb){
       HttpConnection::openCallback_=cb;
    }
    void setWeakEntryPtr(const boost::any& ptr){
        weakEntry_=ptr;
    }
    static int getFileFd(std::string& filename){
        return HttpConnection::openCallback_(filename);
    }
    boost::any getWeakEntryPtr()const{
        return weakEntry_;
    }
    EventLoop* getLoop(){
        return loop_;
    }

private:
    enum StateE
    {
        //双方Fin收到确认（POLLHUP）（handleClose执行）
        kDisconnected,
        //连接创建中（对象创建，Established回调未执行）
        kConnecting,
        //连接已建立（Established回调执行）
        kConnected,
        //shutdown执行，发送Fin（或者打算发送fin）
        kDisconnecting
    };

    void setState(StateE s){state_=s;}
    //注册channel回调
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    //void realSend(const char*,size_t);
    void realSend(SendMsg* message);
    EventLoop *loop_;
    const std::string name_;
    //根据状态处理event
    StateE state_;

    std::unique_ptr<Channel> channel_;
    std::unique_ptr<Socket>connfd_;
    Buffer inputBuffer_;
    
    //Buffer outputBuffer_;
    //实现零拷贝传输
    std::list<SendMsg> outputBuffer_;
    InetAddress peerAddr_;
    InetAddress localAddr_;
    HttpContext context_;
    boost::any weakEntry_;
    //连接主动调用，要求服务器关闭连接（erase，share_ptr计数归0，连接析构）
    CloseCallback closeCallback_;
    //连接建立后调用，调用server的onConnection函数，用于增加定时器，当此链接超时时踢掉
    ConnectionCallback connectionCallback_;
    //handleRead调用，调用server的onMessage函数，用于修改定时器
    MessageCallback messageCallback_;
    //调用server的newFileOpenCallback，减少open调用次数
    static FileOpenCallback openCallback_;
};

#endif