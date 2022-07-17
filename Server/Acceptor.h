#ifndef XACCEPTOR_H
#define XACCEPTOR_H
#include <functional>
#include <arpa/inet.h>
#include<memory>
#include"Socket.h"
#include"InetAddress.h"
#include"Channel.h"

class EventLoop;
//负责创建监听socket，创建新连接
class Acceptor{
    typedef std::function<void(int,InetAddress)>NewConnectionCallback;
    public:

    Acceptor(EventLoop*,const InetAddress&);
    ~Acceptor();
    void setNewConnectionCallback(const NewConnectionCallback& cb){
        newConnectionCallback_=cb;
    }
    //创建监听队列
    void listen(int size=5);

    //int accept(InetAddress& peerAddr);
    void accept();
    private:
    EventLoop* loop_;
    InetAddress addr_;
    Socket listen_fd;
    Channel channel_;
    //提前占一个文件描述符，指向/dev/null，EMFILE错误（文件描述符耗尽）时，可使用此fd解决
    int idleFd_;
    NewConnectionCallback newConnectionCallback_;
};
#endif