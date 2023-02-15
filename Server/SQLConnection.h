#ifndef XSQLCONNECTION_H
#define XSQLCONNECTION_H
#include<mysql/mysql.h>
#include<string>
#include<memory>

class SQLpool;
class Query;
//RAII封装mysql连接，创建对象即连接，析构才断开
class SQLConnection{
    friend Query;
    public:
    SQLConnection(std::string host,std::string user,std::string passwd,std::string db_name);
    ~SQLConnection();
    private:
    //句柄，一切操作基于此机构
    MYSQL* mysql_;
    //指向结果集，即查询结果
    MYSQL_RES* result_=NULL;

    
};
//用于语句执行，避免直接释放MYSQL_RES*,创建时从连接池中获取连接，析构时归还连接
class Query{
    using SqlPtr=std::unique_ptr<SQLConnection>;
    public:
    Query();
    //执行sql语句，返回结果集大小,result设置为指向结果集
    long long query(MYSQL_RES* &result,std::string sql);
    ~Query();
    //判断是否获取连接
    bool getConnection(){return conn_!=nullptr;}
    private:
    SqlPtr conn_;
    SQLpool* pool_;
};

#endif
