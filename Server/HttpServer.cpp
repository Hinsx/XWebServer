#include "HttpServer.h"
#include "Acceptor.h"
#include "Threadpool.h"
#include "EventLoop.h"
#include "SQLpool.h"
#include<boost/any.hpp>

HttpServer::HttpServer(EventLoop *loop, std::string name,const char* ip,int port,int idleSeconds,int maxConnectionNums) : loop_(loop),
                                                                        name_(name),
                                                                        //localAddr_("127.0.0.1",1000),
                                                                        localAddr_(ip,port),
                                                                        ipPort_(InetAddress::addrToIpPort(localAddr_)),
                                                                        acceptor(new Acceptor(loop_, localAddr_)),
                                                                        pool_(Threadpool::init(loop_)),
                                                                        connectionBuckets_(idleSeconds),
                                                                        nextConnId_(1),
                                                                        maxConnectionNums_(maxConnectionNums)
{
    acceptor->setNewConnectionCallback(std::bind(&HttpServer::newConnnectionCallback, this, std::placeholders::_1, std::placeholders::_2));
    //每秒执行一次ontimer
    loop_->runEvery(1.0,std::bind(&HttpServer::onTimer,this));
    connectionBuckets_.resize(idleSeconds);
    LOG_TRACE << "HttpServer init successfully!";
}
HttpServer::~HttpServer()
{
}

void HttpServer::start()
{
    LOG_TRACE << "HttpServer " << name_ << " start.";

    pool_->start();
    SQLpool::getInstance();
    acceptor->listen();
}
void HttpServer::newConnnectionCallback(int connfd, InetAddress peerAddr)
{
    if(connections_.size()<maxConnectionNums_)
    {
        LOG_TRACE << "New connection came.Connfd = " << connfd;
        EventLoop *ioLoop = pool_->getNextLoop();
        //构造连接名称（key）
        char buf[64];
        snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
        ++nextConnId_;
        string connName = name_ + buf;

        HttpConnectionPtr conn(new HttpConnection(ioLoop,
                                                connName,
                                                connfd,
                                                localAddr_,
                                                peerAddr));
        conn->setCloseCallback(std::bind(&HttpServer::removeConnection, this, std::placeholders::_1));
        conn->setConnectionCallback(std::bind(&HttpServer::onConnection,this,std::placeholders::_1));
        conn->setMeesageCallback(std::bind(&HttpServer::onMessage,this,std::placeholders::_1));
        connections_[connName] = conn;

        LOG_DEBUG << "new connection->[" << peerAddr.toIpPort() << "]";
        ioLoop->queueInLoop(std::bind(&HttpConnection::connectEstablished, conn));        
    }
    //关闭连接
    else
    {
        LOG_INFO<<"Too many connections.";
        close(connfd);
    }

}
void HttpServer::removeConnection(const HttpConnectionPtr &conn)
{
    loop_->queueInLoop(std::bind(&HttpServer::removeConnectionInLoop, this, conn));
}
void HttpServer::removeConnectionInLoop(const HttpConnectionPtr &conn)
{
    size_t n = connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&HttpConnection::connectDestroyed, conn));
}

void HttpServer::onConnection(const HttpConnectionPtr& conn){
    EntryPtr entry(new Entry(conn));
    connectionBuckets_.back().insert(entry);
    /*将指向这个entry的weakptr保存起来，当连接收到信息时，
    可以根据weakptr升级为shareptr，保证与entry指向同一个对象。
    不能保存强引用，否则Entry永远不会被析构也就永远不会调用连接的shutdown，除非连接本身被析构
    */
    WeakEntryPtr weakEntry(entry);
    conn->setWeakEntryPtr(weakEntry);
}

void HttpServer::onMessage(const HttpConnectionPtr& conn){
    //boost::any_cast — Custom keyword cast for extracting a value of a given type from an any.
    WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getWeakEntryPtr()));
    EntryPtr entry(weakEntry.lock());
    //如果提升失败，说明连接是在超时的情况下收到信息，那就不更新计时
    if(entry){
        connectionBuckets_.back().insert(entry);
    }
}

void HttpServer::onTimer(){
    connectionBuckets_.push_back(Bucket());
}