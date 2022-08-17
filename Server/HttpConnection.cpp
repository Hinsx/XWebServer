#include "HttpConnection.h"
#include "HttpResponse.h"
#include "../Log/Logger.h"
#include "Channel.h"
#include "Socket.h"
#include "SQLConnection.h"
#include <unistd.h>
#include <sys/stat.h>

extern const char *FILE_PATH;
HttpConnection::FileOpenCallback HttpConnection::openCallback_;

//根据文件后缀设置相应的content-type
void setContentType(HttpResponse &response, const string &filename)
{
    int begin = filename.find_last_of('.', filename.length() - 1);
    const char *postorder = filename.c_str() + begin + 1;
    if (strcmp(postorder, "html") == 0)
    {
        response.setContentType("text/html; charset=utf-8");
    }
    else if (strcmp(postorder, "jpg") == 0)
    {
        response.setContentType("image/jpeg");
    }
    else if (strcmp(postorder, "png") == 0)
    {
        response.setContentType("image/png");
    }
    else if (strcmp(postorder, "css") == 0)
    {
        response.setContentType("text/css");
    }
    else if (strcmp(postorder, "js") == 0)
    {
        response.setContentType("application/x-javascript");
    }
}
//请求文件
void acquireFile(HttpResponse &response, std::string &filename)
{
    //检查访问的文件的属性
    struct stat attribute;

    //文件或目录不存在
    if (stat(filename.c_str(), &attribute) < 0)
    {
        response.setStatusCode(HttpResponse::k404NotFound);
        response.setStatusMessage("NotFound");
    }
    //文件或目录不允许其他人读取(只有root可读)
    else if (!(attribute.st_mode & S_IROTH))
    {
        response.setStatusCode(HttpResponse::k403ForBidden);
        response.setStatusMessage("ForBidden");
    }

    //访问路径是一个目录
    else if (S_ISDIR(attribute.st_mode))
    {
        response.setStatusCode(HttpResponse::k400BadRequest);
        response.setStatusMessage("BadRequest");
    }
    //正常访问
    else
    {
        response.setStatusCode(HttpResponse::k200Ok);
        response.setStatusMessage("OK");
        // response.setContentType("image/jpeg");
        setContentType(response, filename);
        response.addHeader("Server", "xWebServer");
        //response.setFile(filename, attribute.st_size);
        int filefd=HttpConnection::getFileFd(filename);
        response.setFile(filefd, attribute.st_size);
        //response.setFile(-1, 0);
        // //读取文件到body
        // FILE* file=fopen(filename.c_str(),"rb");
        // char output[buf.st_size+1];
        // string body;
        // while(!feof(file))
        // {
        //     memset(output,'\0',sizeof(output));
        //     size_t n=fread(output,sizeof(char),sizeof(output)-1,file);
        //     response.appendToBody(output,n);
        // }
        // fclose(file);
    }
}

//处理GET请求
void handleGET(const HttpRequest &req, HttpResponse &response)
{
    //访问的文件
    std::string filename(FILE_PATH);
    //访问首页
    if (req.path() == "/" || req.path() == "http://120.76.192.202:9006/")
    {
        filename += "/login.html";
    }
    else
    {
        filename += req.path();
    }
    acquireFile(response, filename);
}
//处理POST请求
void handlePOST(const HttpRequest &req, HttpResponse &response)
{
    //注册活动,输入用户名和密码
    if (req.path() == "/registe")
    {
        MYSQL_RES *result;
        int row = -1;
        {
            Query query;
            if (query.getConnection())
            {
                row = query.query(result, "SELECT user_name FROM users WHERE user_name='" + req.getValueByKey("name") + "';");
                //注册的用户不存在，可以注册
                if (row == 0)
                {
                    query.query(result, "INSERT INTO users (user_name,user_passwd) VALUES ('" + req.getValueByKey("name") + "','" + req.getValueByKey("password") + "');");
                }
            }
        }
        //系统繁忙，无法获取数据库连接
        if (row == -1)
        {
            response.setStatusCode(HttpResponse::k503ServiceUnavailable);
            response.setStatusMessage("Service Unavailable");
        }
        //已执行注册，返回登录界面
        else if (row == 0)
        {
            response.setStatusCode(HttpResponse::k200Ok);
            response.setStatusMessage("OK");
            response.setContentType("text/plain");
            response.addHeader("Server", "xWebServer");
            response.setBody("{\"status\":\"success\"}");
        }
        //注册的用户已经存在，返回提醒
        else if (row > 0)
        {
            response.setStatusCode(HttpResponse::k200Ok);
            response.setStatusMessage("OK");
            response.setContentType("text/plain");
            response.addHeader("Server", "xWebServer");
            response.setBody("{\"status\":\"exist\"}");
        }
    }
    //登录活动
    else if (req.path() == "/login")
    {
        MYSQL_RES *result = NULL;
        int row = -1;
        {
            Query query;
            if (query.getConnection())
            {
                row = query.query(result, "SELECT user_name FROM users WHERE user_name='" + req.getValueByKey("name") + "' AND user_passwd='" + req.getValueByKey("password") + "';");
            }
        }

        //系统繁忙，无法获取数据库连接
        if (row == -1)
        {
            response.setStatusCode(HttpResponse::k503ServiceUnavailable);
            response.setStatusMessage("Service Unavailable");
        }
        //不存在此用户，检查用户名或者密码
        else if (row == 0)
        {
            response.setStatusCode(HttpResponse::k200Ok);
            response.setStatusMessage("OK");
            response.setContentType("text/plain");
            response.addHeader("Server", "xWebServer");
            response.setBody("{\"status\":\"noexist\"}");
        }
        //登录的用户存在
        else if (row > 0)
        {
            assert(row == 1);

            response.setStatusCode(HttpResponse::k200Ok);
            response.setStatusMessage("OK");
            response.setContentType("text/plain");
            response.addHeader("Server", "xWebServer");
            response.setBody("{\"status\":\"success\",\"path\":\"/index.html\"}");
        }
    }
}

//处理各种请求
void handleReq(const HttpRequest &req, HttpResponse &response)
{
    if (req.methodString() == "GET")
    {
        handleGET(req, response);
    }
    else if (req.methodString() == "POST")
    {
        handlePOST(req, response);
    }
}
HttpConnection::HttpConnection(EventLoop *loop,
                               const string &nameArg,
                               int sockfd,
                               const InetAddress &localAddr,
                               const InetAddress &peerAddr)
    : loop_(loop),
      name_(nameArg),
      state_(kConnecting),
      channel_(new Channel(loop, sockfd)),
      connfd_(new Socket(sockfd)),
      peerAddr_(peerAddr),
      localAddr_(localAddr)

{
    channel_->setReadCallBack(
        std::bind(&HttpConnection::handleRead, this));
    channel_->setWriteCallBack(
        std::bind(&HttpConnection::handleWrite, this));
    channel_->setCloseCallBack(
        std::bind(&HttpConnection::handleClose, this));
    channel_->setErrorCallBack(
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
    if (getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        optval = errno;
    }
    int err = optval;
    // LOG_ERROR << "HttpConnection::handleError [" << name_
    //           << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
//调用时机
// 1.客户端（先于服务器）主动关闭写端，收到FIN包，导致read=0，在handleRead调用（此时state_=kConnected)
// 2.服务器回复报文后调用shutdown（短连接/报文错误），客户端收到FIN包回复FIN包，产生POLLHUP，在channel.handleEvent中调用(此时state_=kDisconnecting)
void HttpConnection::handleClose()
{
    setState(kDisconnected);
    channel_->disableAll();
    HttpConnectionPtr guardThis(shared_from_this());
    //此行执行后，连接析构
    closeCallback_(guardThis);
}

//不去特判是数据到来还是客户端写端挂起，当POLLIN和POLLRDHUP同时发生，首先读取完整数据（只可能是read>0），等待下一次监听返回只有POLLRDHUP，此时read返回0
//考虑有没有可能：POLLIN和POLLRDHUP同时发生，但是数据不完整（read<0)，而到下次poll时新数据仍未到达，此时因为收到FIN报文而直接handleRead->read=0，关闭连接，导致剩下的数据没有完全读取？
//不可能，写端挂起（发fin），首先就需要对方写缓冲区全部发出后，再发一个fin报文，若fin先于部分数据到达，因为tcp报文有先后顺序，会先排序再交给应用层，所以fin会等到所有报文数据到达再被读取
void HttpConnection::handleRead()
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);

    //读取到报文目前全部数据，执行解析
    if (n > 0)
    {
        //有数据到来则更新定时器
        messageCallback_(shared_from_this());
        //printf("%s", inputBuffer_.peek());
        //解析失败，报文格式错误（若是因为报文不完整，n<0，不会进入此分支）
        if (!context_.parseRequest(&inputBuffer_))
        {

            send("HTTP/1.1 400 Bad Request\r\n\r\n");
            //具体逻辑是什么？
            shutdown();
        }
        //解析成功
        if (context_.gotAll())
        {

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
            LOG_TRACE << "Ready to prepare response. Method=" << req.methodString() << " URL=" << req.path();

            //填充报文
            handleReq(req, response);

            //创建临时buffer，若无法全部传入socket缓冲再保存至连接的output buffer
            // Buffer buf;
            // response.appendToBuffer(&buf);
            SendMsg msg(response.getFilefd(), response.getFilesize());
            response.appendToBuffer_(&msg);
            send(&msg);
            // send(&buf);
            

            //如果是短连接
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
        LOG_TRACE << "Peer shut write down.";
        handleClose();
    }
    //发生错误(不是EAGAIN这类错误，因为通知有数据到达)
    else
    {
        errno = savedErrno;
        // LOG_SYSERR << "something wrong when reading datas from fd";
        handleError();
    }
}

// void HttpConnection::send(Buffer *buf)
// {
//     //为什么有可能send时连接状态不为connected？
//     //长连接情况下，第一个报文格式错误，调用shutdown，状态修改为kDisConnecting，
//     //此时第二个报文到来，则不予回复，否则写端可能很长时间无法关闭
//     if (state_ == kConnected)
//     {
//         realSend(buf->peek(), buf->readableBytes());
//         buf->retrieveAll();
//     }
// }

void HttpConnection::send(SendMsg* message)
{
    //为什么有可能send时连接状态不为connected？
    //长连接情况下，第一个报文格式错误，调用shutdown，状态修改为kDisConnecting，
    //此时第二个报文到来，则不予回复，否则写端可能很长时间无法关闭
    if (state_ == kConnected)
    {
        realSend(message);
    }
}

void HttpConnection::send(const char *data, size_t len)
{
    if (state_ == kConnected)
    {
        SendMsg message(-1);
        Buffer *buffer = message.getBuffer();
        buffer->append(data, len);
        realSend(&message);
    }
}
// void HttpConnection::realSend(const char *data, size_t len)
// {
//     ssize_t nwrote = 0;
//     //剩余未发送数据的长度
//     size_t remaining = len;
//     bool faultError = false;
//     /*
//       直接尝试向缓冲区写入数据，需要注意避免乱序发送
//       1.如果outputbuffer中有数据，说明之前写入数据不完全，此时直接写入会乱序
//       2.第一个条件有点迷糊？buffer为空不就说明没有未发送的数据吗？
//     */
//     if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
//     {
//         // std::string message;
//         // message.assign(data,len);
//         // LOG_TRACE<<"------------------The message is:\n"<<message;

//         // fd是非阻塞的，可以直接尝试write
//         nwrote = write(channel_->fd(), data, len);
//         //如果数据写入（部分）成功，0表示对端读端关闭，之后再写会造成EPIPE错误
//         //读端关闭时本机是收不到通知的（写端关闭会收到fin），只能用wirte来发现
//         if (nwrote > 0)
//         {
//             remaining = len - nwrote;
//         }
//         //返回-1/0，发生错误/对端关闭
//         else
//         {
//             nwrote = 0; //相当于写入0字节
//             if (errno != EWOULDBLOCK)
//             {
//                 LOG_SYSERR << "something wrong when writing to fd.";
//                 // EPIPE：对面连接关闭，关闭了读端，收到信息则返回RST
//                 // ECONNRESET：连接重置，具体是怎么错误的？
//                 if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
//                 {
//                     faultError = true;
//                     //对端已经挂起读端，那么我们也没必要再写入了
//                     shutdown();
//                 }
//             }
//         }
//     }

//     // 1.因为阻塞导致没有完全写入 / 因为之前数据未发送，此次数据为了避免乱序而没有发送
//     // 2.读端未关闭/连接未重置
//     if (!faultError && remaining > 0)
//     {
//         size_t oldLen = outputBuffer_.readableBytes();
//         // output buffer的关键作用
//         outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
//         if (!channel_->isWriting())
//         {
//             channel_->enableWriting();
//         }
//     }
// }

void HttpConnection::realSend(SendMsg *message)
{

    bool faultError = false;
    /*
      直接尝试向缓冲区写入数据，需要注意避免乱序发送
      如果outputbuffer中有数据，说明之前写入数据不完全，此时直接写入会乱序
    */
    if (outputBuffer_.empty())
    {

        int ret = message->writeToSocket(channel_->fd());
        // ret>0:数据(可能只有部分）写入成功
        // ret<0:发生错误,其中epipe错误经过两次write发现
        if (ret < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "something wrong when writing to fd.";
                // EPIPE：对面连接关闭，关闭了读端，收到信息则返回RST
                // ECONNRESET：连接重置，具体是怎么错误的？
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    faultError = true;
                    //对端已经挂起读端，那么我们也没必要再写入了
                    shutdown();
                }
            }
        }
    }

    // 1.读端未关闭/连接未重置
    // 2.因为阻塞导致没有完全写入 / 因为之前数据未发送，此次数据为了避免乱序而没有发送
    if (!faultError && !message->isWriteAll())
    {
        outputBuffer_.push_back(*message);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
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
        if (!channel_->isWriting())
        {
            LOG_TRACE << "shut down write at " << channel_->fd();
            ::shutdown(channel_->fd(), SHUT_WR);
        }
    }
}

// void HttpConnection::handleWrite()
// {
//     //可能等待可写时，客户端关闭，已经执行了handleClose，此时写数据已经没有意义
//     if (channel_->isWriting())
//     {
//         ssize_t n = write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());

//         if (n > 0)
//         {
//             outputBuffer_.retrieve(n);
//             if (outputBuffer_.readableBytes() == 0)
//             {
//                 //因为epoll使用了LT模式，既然已经无需写数据，则取消读事件避免busyloop
//                 //即使是ET模式也需要此行，否则也会多几次通知（tcp读缓冲区逐渐发送，直到发送结束都会不断通知）
//                 channel_->disableWriting();

//                 //为什么channel对write感兴趣，连接状态却是disconnecting
//                 // shutdown调用因为buffer有数据而放弃挂起写端,现在可以挂起了
//                 if (state_ == kDisconnecting)
//                 {
//                     ::shutdown(channel_->fd(), SHUT_WR);
//                 }
//             }
//         }
//     }
// }

void HttpConnection::handleWrite()
{
    //可能等待可写时，客户端关闭，已经执行了handleClose，此时写数据已经没有意义
    if (channel_->isWriting())
    {
        while (!outputBuffer_.empty())
        {
            SendMsg &message = outputBuffer_.front();
            int ret = message.writeToSocket(channel_->fd());
            //缓冲区已满/对端挂起/连接重置，放弃写入
            if (ret < 0)
            {
                if (errno != EWOULDBLOCK)
                {
                    LOG_SYSERR << "something wrong when writing to fd.";
                    // EPIPE：对面连接关闭，关闭了读端，收到信息则返回RST
                    // ECONNRESET：连接重置，具体是怎么错误的？
                    if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                    {
                        //对端已经挂起读端，那么我们也没必要再写入了
                        shutdown();
                    }
                }
                break;
            }
            //如果写入完成，执行下一个写入，否则直接对当前信息再次写入，基本会造成ewouldblock导致停止循环
            if (message.isWriteAll())
            {
                outputBuffer_.pop_front();
            }
        }
        //数据全部写完
        if (outputBuffer_.empty())
        {
            //因为epoll使用了LT模式，既然已经无需写数据，则取消读事件避免busyloop
            //即使是ET模式也需要此行，否则也会多几次通知（tcp读缓冲区逐渐发送，直到发送结束都会不断通知）
            channel_->disableWriting();

            //为什么channel对write感兴趣，连接状态却是disconnecting
            // shutdown调用因为buffer有数据而放弃挂起写端,现在可以挂起了
            if (state_ == kDisconnecting)
            {
                ::shutdown(channel_->fd(), SHUT_WR);
            }
        }
    }
}

void HttpConnection::connectEstablished()
{
    setState(kConnected);
    //为何channel_要用weakptr绑定this？什么时候会出现"在handleEvent时连接析构"的情况？
    // serve主线程调用server析构，将存在的连接逐个删除
    // HttpConnectionPtr conn=shared_from_this();
    channel_->tie(shared_from_this());
    channel_->enableReading();
    connectionCallback_(shared_from_this());
}

void HttpConnection::connectDestroyed()
{

    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
    }
    channel_->remove();
}