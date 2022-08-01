#ifndef XSQLPOOL_H
#define XSQLPOOL_H
#include<pthread.h>
#include<memory>
#include<list>
#include<string>
#include"Lock.h"
//单例模式实现连接池,基于list分配连接

class SQLConnection;
class SQLpool{
    public:
    //设置连接池的上限
    static void setPoolMaxSize(int num){
        maxConnNums_=num<=5?num:5;

    }
    //设置连接池的下限void
    static void setPoolMinSize(int num){
        minConnNums_=num>=2?num:2;
    }
    //设置连接参数
    static void setConnectArg(std::string host,std::string user,std::string passwd,std::string db_name){
        host_=host;
        user_=user;
        passwd_=passwd;
        db_name_=db_name;
    }
    //初始化接口,获取实例，创建连接池
    static SQLpool* getInstance(){
        static SQLpool pool;
        return &pool;
    }
    //获取目前可用的连接
    SQLConnection* getConnection();
    //释放连接，返回连接池
    void releaseConnection(SQLConnection*);
    private:
    //初始化连接池
    SQLpool();
    ~SQLpool();
    //用链表组织数据库连接
    std::list<SQLConnection*>conns_;
    //互斥获取/释放连接
    MutexLock mutex;
    Condition cond;

    static int maxConnNums_;
    static int minConnNums_;
    static std::string host_;
    static std::string user_;
    static std::string passwd_;
    static std::string db_name_;
};

#endif