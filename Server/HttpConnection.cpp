#include "HttpConnection.h"
#include "HttpResponse.h"
#include "../Log/Logger.h"

#include <unistd.h>

HttpConnection::HttpConnection(EventLoop *loop,
                               const string &nameArg,
                               int sockfd,
                               const InetAddress &localAddr,
                               const InetAddress &peerAddr)
    : loop_(loop),
      name_(nameArg),
      state_(kConnecting),
      channel_(loop, sockfd),
      localAddr_(localAddr),
      peerAddr_(peerAddr)
{
    channel_.setReadCallBack(
        std::bind(&HttpConnection::handleRead, this));
    channel_.setWriteCallBack(
        std::bind(&HttpConnection::handleWrite, this));
    channel_.setCloseCallBack(
        std::bind(&HttpConnection::handleClose, this));
    channel_.setErrorCallBack(
        std::bind(&HttpConnection::handleError, this));
    LOG_DEBUG << "HttpConnection create.[" << name_ << "] at " << this
              << " fd=" << sockfd;
}

HttpConnection::~HttpConnection()
{
    LOG_DEBUG << "Connection destory.";
}
void HttpConnection::handleError()
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    if (getsockopt(channel_.fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        optval = errno;
    }
    int err = optval;
    LOG_ERROR << "HttpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
//调用时机
// 1.客户端（先于服务器）主动关闭写端，收到FIN包，导致read=0，在handleRead调用（此时state_=kConnected)
// 2.服务器回复报文后调用shutdown（短连接/报文错误），客户端收到FIN包回复FIN包，产生POLLHUP，在channel.handleEvent中调用(此时state_=kDisconnecting)
void HttpConnection::handleClose()
{
    setState(kDisconnected);
    channel_.disableAll();
    HttpConnectionPtr guardThis(shared_from_this());
    //此行执行后，连接析构
    closeCallback_(guardThis);
}

//不去特判是数据到来还是客户端写端挂起，当POLLIN和POLLRDHUP同时发生，首先读取完整数据（只可能是read>0），等待下一次监听返回只有POLLRDHUP，此时read返回0
//考虑有没有可能：POLLIN和POLLRDHUP同时发生，但是数据不完整（read<0)，而到下次poll时新数据仍未到达，此时因为收到FIN报文而直接handleRead->read=0，关闭连接，导致剩下的数据没有完全读取？
//不可能，写端挂起（发fin），首先就需要对方写缓冲区全部发出后，再发一个fin报文，若fin先于部分数据到达，因为tcp报文有先后顺序，会先排序再交给应用层，所以fin会等到所有报文数据到达再被读取
void HttpConnection::handleRead()
{
    LOG_TRACE<<"start read--------------";
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_.fd(), &savedErrno);
    // std::string post;
    // post.assign(inputBuffer_.peek(),inputBuffer_.readableBytes());
    // LOG_TRACE<<"post is:\n"<<post;
    //读取到报文目前全部数据，执行解析
    if (n > 0)
    {   
        LOG_TRACE<<"---------Get all post,starting explaining...----------";
        //解析失败，报文格式错误（若是因为报文不完整，n<0，不会进入此分支）
        if (!context_.parseRequest(&inputBuffer_))
        {
            LOG_TRACE<<"---------bad request...----------";

            send("HTTP/1.1 400 Bad Request\r\n\r\n");
            //具体逻辑是什么？
            shutdown();
        }
        //解析成功
        if (context_.gotAll())
        {
            LOG_TRACE<<"---------Explained----------";

            const HttpRequest &req = context_.request();
            //获取连接状态（长连接/短连接）
            // http1.1,默认开启长连接，关闭短连接需要额外加入报文头部字段"Connection:close"字段
            // http1.0,默认关闭长连接，开启长连接需要额外加入报文头部字段"Connection:Keep-Alive"字段
            const string &connection = req.getHeader("Conection");
            //判断是否为短连接
            //若是http1.1，则可直接找到close字段
            //若是http1.0，则需判断为1.0版本且没有开启keep-alive
            bool close = (connection == "close" || req.getVersion() == HttpRequest::kHttp10 && connection != "keep-alive");

            //准备回复报文
            HttpResponse response(close);
            LOG_DEBUG << "Ready to prepare response. Method=" << req.methodString() << " URL=" << req.path();

            //访问首页
            if (req.path() == "/")
            {
                response.setStatusCode(HttpResponse::k200Ok);
                response.setStatusMessage("OK");
                response.setContentType("text/html");
                response.addHeader("Server", "xWebServer");
                //考虑用零拷贝
                response.setBody("<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>This is title</title></head><body><h1>Hello</h1>Welcome to xWebserver</body></html>");
            }

            //不支持访问其他文件
            else
            {
                response.setStatusCode(HttpResponse::k404NotFound);
                response.setStatusMessage("Not Found");
                response.setCloseConnection(true);
            }

            //创建临时buffer，若无法全部传入socket缓冲再保存至连接的output buffer
            Buffer buf;
            response.appendToBuffer(&buf);
            send(&buf);

            //如果是短连接，或者访问不存在的资源，则关闭
            if (response.closeConnection())
            {
                //如果send的部分数据存在buffer呢？
                shutdown();
            }
            //清空报文，等待下一个报文
            context_.reset();
        }
    }
    // POLLRDHUP
    else if (n == 0)
    {
        LOG_TRACE<<"Peer shut write down.";
        handleClose();
    }
    //发生错误(不是EAGAIN这类错误，因为通知有数据到达)
    else
    {
        errno = savedErrno;
        LOG_SYSERR << "something wrong when reading datas from fd";
        handleError();
    }
}

void HttpConnection::send(Buffer *buf)
{
    //为什么有可能send时连接状态不为connected？
    //长连接情况下，第一个报文格式错误，调用shutdown，状态修改为kDisConnecting，
    //此时第二个报文到来，则不予回复，否则写端可能很长时间无法关闭
    if (state_ == kConnected)
    {
        realSend(buf->peek(), buf->readableBytes());
        buf->retrieveAll();
    }
}
void HttpConnection::send(const char *data, size_t len)
{
    if (state_ == kConnected)
    {
        realSend(data, len);
    }
}
void HttpConnection::realSend(const char *data, size_t len)
{
    ssize_t nwrote = 0;
    //剩余未发送数据的长度
    size_t remaining = len;
    bool faultError = false;
    //在进入realsend之前已经判断过状态（connected）了，为何还要判断呢？
    if (state_ == kDisconnected)
    {
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    /*
      直接尝试向缓冲区写入数据，需要注意避免乱序发送
      1.如果outputbuffer中有数据，说明之前写入数据不完全，此时直接写入会乱序
      2.第一个条件有点迷糊？buffer为空不就说明没有未发送的数据吗？
    */
    if (!channel_.isWriting() && outputBuffer_.readableBytes() == 0)
    {
        //std::string message;
        //message.assign(data,len);
        //LOG_TRACE<<"------------------The message is:\n"<<message;

        // fd是非阻塞的，可以直接尝试write
        nwrote = write(channel_.fd(), data, len);
        //如果数据写入（部分）成功，返回0和EWOULDBLOCK的区别？
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
        }
        //返回-1，发生错误
        else // nwrote < 0
        {
            nwrote = 0; //相当于写入0字节
            if (errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "something wrong when writing to fd.";
                // EPIPE：对面连接关闭，关闭了读端，收到信息则返回RST
                // ECONNRESET：连接重置，具体是怎么错误的？
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    faultError = true;
                }
            }
        }
    }

    // 1.因为阻塞导致没有完全写入
    // 2.因为之前数据未发送，此次数据为了避免乱序而没有发送
    if (!faultError && remaining > 0)
    {
        size_t oldLen = outputBuffer_.readableBytes();
        // output buffer的关键作用
        outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
        if (!channel_.isWriting())
        {
            channel_.enableWriting();
        }
    }
}

//主动发送FIN报文（挂起写端）,在发送响应报文后(可能没能全部发送，部分存入buffer）关闭连接（HTTP1.0/报文错误）
void HttpConnection::shutdown()
{
    //考虑客户端连续发两个格式错误报文，对第二个至后续的报文，不予回复，也无需再修改状态（第一次错误即shutdown）
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        //若部分数据暂存用户空间，则允许写，等待hanleWrite完成后主动执行挂起
        if (!channel_.isWriting())
        {   LOG_TRACE<<"shut down write at "<<channel_.fd();
            ::shutdown(channel_.fd(), SHUT_WR);
        }
    }
}

void HttpConnection::handleWrite()
{
    //可能等待可写时，客户端关闭，已经执行了handleClose，此时写数据已经没有意义
    if (channel_.isWriting())
    {
        ssize_t n = write(channel_.fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());

        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                //因为epoll使用了LT模式，既然已经无需写数据，则取消读事件避免busyloop
                //即使是ET模式也需要此行，否则也会多几次通知（tcp读缓冲区逐渐发送，直到发送结束都会不断通知）
                channel_.disableWriting();

                //为什么channel对write感兴趣，连接状态却是disconnecting
                // shutdown调用因为buffer有数据而放弃挂起写端,现在可以挂起了
                if (state_ == kDisconnecting)
                {
                    ::shutdown(channel_.fd(), SHUT_WR);
                }
            }
        }
    }
}
void HttpConnection::connectEstablished()
{
    setState(kConnected);
    //为何channel_要用weakptr绑定this？什么时候会出现"在handleEvent时连接析构"的情况？
    // serve主线程调用server析构，将存在的连接逐个删除
    channel_.tie(shared_from_this());
    channel_.enableReading();
}

void HttpConnection::connectDestroyed()
{

    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_.disableAll();
    }
    channel_.remove();
}