#include "HttpServer.h"
#include "Acceptor.h"
#include "Threadpool.h"
#include "../Log/Logger.h"
#include "EventLoop.h"
#include "HttpConnection.h"

HttpServer::HttpServer(EventLoop *loop, std::string name,bool epoll) : loop_(loop),
                                                                        name_(name),
                                                                        localAddr_("127.0.0.1",1000),
                                                                        //localAddr_(9006),
                                                                        ipPort_(InetAddress::addrToIpPort(localAddr_)),
                                                                        acceptor(new Acceptor(loop_, localAddr_)),
                                                                        pool_(Threadpool::init(loop_)),
                                                                        
                                                                        nextConnId_(1),
                                                                        epoll_(epoll)
{
    acceptor->setNewConnectionCallback(std::bind(&HttpServer::newConnnectionCallback, this, std::placeholders::_1, std::placeholders::_2));
    LOG_TRACE << "HttpServer init successfully!";
}
HttpServer::~HttpServer()
{
}

void HttpServer::start()
{
    LOG_TRACE << "HttpServer " << name_ << " start.";

    pool_->start();

    acceptor->listen();
}
void HttpServer::newConnnectionCallback(int connfd, InetAddress peerAddr)
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
    connections_[connName] = conn;

    LOG_DEBUG << "new connection->[" << peerAddr.toIpPort() << "]";
    ioLoop->queueInLoop(std::bind(&HttpConnection::connectEstablished, conn));
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