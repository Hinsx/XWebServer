#include "SQLConnection.h"
#include "SQLpool.h"
#include "../Log/Logger.h"
using std::string;
SQLConnection::SQLConnection(string host, string user, string passwd, string db_name)
{
    //初始化句柄
    mysql_ = mysql_init(NULL);
    if (mysql_ == NULL)
    {
        LOG_SYSFATAL << "Failed in mysql_init";
    }
    //连接数据库
    mysql_ = mysql_real_connect(mysql_, host.c_str(), user.c_str(), passwd.c_str(), db_name.c_str(), 0, NULL, 0);
    if (mysql_ == NULL)
    {
        LOG_SYSFATAL << "Failed in mysql_real_connect";
    }
}
SQLConnection::~SQLConnection()
{
    if (mysql_ != NULL)
    {
        mysql_close(mysql_);
    }
}
Query::Query():pool_(SQLpool::getInstance())
{  
    conn_=pool_->getConnection();
}
Query::~Query()
{
    if (conn_ != nullptr)
    {
        if (conn_->result_ != NULL)
        {
            mysql_free_result(conn_->result_);
            conn_->result_=NULL;
            pool_->releaseConnection(std::move(conn_));
        }
    }
}

long long Query::query(MYSQL_RES *&result, string sql)
{
    if (conn_ != nullptr)
    {
        if (conn_->result_ != NULL)
        {
            mysql_free_result(conn_->result_);
            conn_->result_ = NULL;
        }
        mysql_query(conn_->mysql_, sql.c_str());
        conn_->result_ = mysql_store_result(conn_->mysql_);
        result = conn_->result_;
        /*
        有可能影响的结果集大小为0（insert/select结果为0），那么mysql_store_result返回空指针
        用空指针操作mysql_num_rows()将会造成segment fault
        */
        return result!=NULL?mysql_num_rows(result):0;
    }
    return -1;
}