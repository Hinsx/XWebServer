#ifndef XCONFIG_H
#define XCONFIG_H
#include <string>
#include<cstring>
#include <unistd.h>
class Config
{
private:
    // io线程数量(从reactor数量)
    int threadNum_ = 2;
    // 数据库连接池数量
    int sqlConnectionNums_ = 3;
    // 日志模式,同步到标准输出流/异步到文件,默认同步
    bool asynclogging = false;
    // 日志级别,TRACE DEBUG INFO （前三级可过滤）ERROR WARN FATAL
    int logLevel_ = 2;
    // 使用epoll模式还是poll模式,默认epoll
    bool mode = true;
    // 服务器名称
    std::string name_ = "XWebServer";
    // 服务器地址,默认绑定到所有网卡
    char *ip_ = nullptr;
    // 服务器端口
    int port_ = 9006;
    // 空闲连接超时时间
    int idle_ = 5;
    // 允许同时处理连接的最大数量
    int connectionNums_ = 5000;
    // 当前资源文件（html/css/js等）所在的路径
    std::string resourcePath; //="/home/alkali/cpp/XWebServer/root";

public:
    Config()
    {
        char buff[255]{0};
        char* ret=getcwd(buff,sizeof(buff));
        if(ret==NULL){
            exit(1);
        }
        resourcePath=buff;
        int index=resourcePath.rfind("XWebServer");
        resourcePath=resourcePath.substr(0,index);
        resourcePath.append("XWebServer/root");
    }

    void parse_arg(int argc, char *argv[]);
    int getThreadNum() { return threadNum_; }
    int getSqlConnectionNum() { return sqlConnectionNums_; }
    bool asynclog() { return asynclogging; }
    int loglevel() { return logLevel_; }
    bool useEpoll() { return mode; }
    std::string serverName() { return name_; }
    char *serverIP() { return ip_; }
    int port() { return port_; }
    int idle() { return idle_; }
    int connectionNums() { return connectionNums_; }
    const char *path() { return resourcePath.c_str(); }
};

#endif