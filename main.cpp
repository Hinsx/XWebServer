#include"Server/HttpServer.h"
#include"Server/Threadpool.h"
#include"Server/EventLoop.h"
#include"Log/Logger.h"
#include"Log/AsyncLogging.h"
#include"Config.h"
using namespace std;
int main(int argc,char** argv){

    Config config;
    config.parse_arg(argc,argv);

    Logger::setLogLevel(config.loglevel());
    if(config.asynclog()){
        AsyncLogging logger("debug",3000);
        logger.start();        
    }
    EventLoop loop(config.useEpoll());
    Threadpool::set_maxThreadNumber(config.getThreadNum());
    HttpServer server(&loop,config.serverName(),config.serverIP(),config.port(),config.idle());
    server.start();
    loop.loop();
    return 0;
}