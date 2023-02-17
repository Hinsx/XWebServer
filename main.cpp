#include"Server/HttpServer.h"
#include"Server/Threadpool.h"
#include"Server/EventLoop.h"
#include"Log/Logger.h"
#include"Log/AsyncLogging.h"
#include"Server/Config.h"
#include"Server/SQLpool.h"
using namespace std;

#ifdef MYTRACE
#include<iostream>
using std::cout;
#endif
//全局变量，文件路径
const char* FILE_PATH;
int main(int argc,char** argv){

    Config config;
    //解析命令
    config.parse_arg(argc,argv);
    //设置访问文件路径
    FILE_PATH=config.path();
    //设置日志等级
    Logger::setLogLevel(config.loglevel());
    //同步/异步日志
    AsyncLogging logger("debug",3000);
    //开启异步日志
    if(config.asynclog()){  
        logger.start();        
    }
    EventLoop loop(config.useEpoll());
    //数据库连接池大小
    SQLpool::setPoolSize(config.getSqlConnectionNum());
    //设置线程池大小
    Threadpool::setThreadNum(config.getThreadNum());
    //设置io复用模式
    Threadpool::setPollMode(config.useEpoll());
    //配置服务器：服务器名称，ip，端口，空闲连接等待事件，最大连接数量
    HttpServer server(&loop,config.serverName(),config.serverIP(),config.port(),config.idle(),config.connectionNums());
    
    #ifdef MYTRACE
    cout<<"Start the server.\n";
    #endif

    server.start();

    #ifdef MYTRACE
    cout<<"Start the loop.\n";
    #endif
    
    loop.loop();
    return 0;
}