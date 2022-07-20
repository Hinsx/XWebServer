#include"Server/HttpServer.h"
#include"Server/Threadpool.h"
#include"Server/EventLoop.h"
#include"Log/Logger.h"
#include"Log/AsyncLogging.h"
using namespace std;
int main(int arg,char** argv){

    Logger::setLogLevel(3);
    //AsyncLogging logger("debug",3000);
    //logger.start();
    LOG_SYSERR << "something wrong when writing to fd.";

    EventLoop loop;
    Threadpool::set_maxThreadNumber(3);
    HttpServer server(&loop,"XuanServer");
    server.start();
    loop.loop();
    return 0;
}