#include"Server/HttpServer.h"
#include"Server/Threadpool.h"
#include"Server/EventLoop.h"
#include"Log/Logger.h"
using namespace std;
int main(int arg,char** argv){

    Logger::setLogLevel(0);

    EventLoop loop;
    Threadpool::set_maxThreadNumber(2);
    HttpServer server(&loop,"XuanServer");
    server.start();
    loop.loop();
    return 0;
}