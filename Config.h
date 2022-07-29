#ifndef XCONFIG_H
#define XCONFIG_H
#include<string>
class Config{
    private:
    //io线程数量(从reactor数量)
    int threadNum_=4;
    //日志模式,同步到标准输出流/异步到文件,默认同步
    bool asynclogging=false;
    //日志级别,TRACE DEBUG INFO （前三级可过滤）ERROR WARN FATAL 
    int logLevel_=0;
    //使用epoll模式还是poll模式,默认epoll
    bool mode=true;
    //服务器名称
    std::string name_="XWebServer";
    //服务器地址
    char* ip_=nullptr;
    //服务器端口
    int port_=9006;
    //空闲连接超时时间
    int idle_=5;
    
    public:
    Config()=default;

    void parse_arg(int argc, char*argv[]);
    int getThreadNum(){return threadNum_;}
    bool asynclog(){return asynclogging;}
    int loglevel(){return logLevel_;}
    bool useEpoll(){return mode;}
    std::string serverName(){return name_;}
    char* serverIP(){return ip_;}
    int port(){return port_;}
    int idle(){return idle_;}

};

#endif