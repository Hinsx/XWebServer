#include "SQLpool.h"
#include "SQLConnection.h"
#include"../Log/Logger.h"
using std::string;

int SQLpool::maxConnectionNums_ = 5;
int SQLpool::ConnectionNums_ ;
string SQLpool::host_="127.0.0.1";
string SQLpool::user_="root";
string SQLpool::passwd_="123";
string SQLpool::db_name_="myweb";

SQLpool::SQLpool() : cond(mutex)
{
    for (int i = ConnectionNums_; i >= 0; i--)
    {
        conns_.push_back(new SQLConnection(host_, user_, passwd_, db_name_));
    }
    LOG_TRACE<<"SQLpool create sccussfully.";
}

SQLConnection *SQLpool::getConnection()
{
    {
        MutexLockGuard guard(mutex);
        //尝试获取一个连接
        if (conns_.empty())
        {
            //获取失败，等待10ms
            cond.waitForSeconds(0.01);
        }
        //等待超时,返回空指针
        if (conns_.empty())
        {
            return nullptr;
        }
        SQLConnection *conn = conns_.back();
        conns_.pop_back();
        return conn;
    }
}

void SQLpool::releaseConnection(SQLConnection *conn)
{
    if (conn != nullptr)
    {
        {
            MutexLockGuard guard(mutex);
            conns_.push_back(conn);
        }
    }
}

SQLpool::~SQLpool()
{
    for (int i = ConnectionNums_; i >= 0; i--)
    {
        SQLConnection *conn = conns_.back();
        delete conn;
        conns_.pop_back();
    }
}