#ifndef MYSQLCONNECTIONPOOL_H
#define MYSQLCONNECTIONPOOL_H

#include <string>
#include <deque>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Condition.h>
#include <muduo/base/Singleton.h>
#include <muduo/base/Logging.h>


using namespace muduo;
using namespace std;

class MySQLConnectionPool : public noncopyable
{
public:
    static MySQLConnectionPool &instance()
    {
        return Singleton<MySQLConnectionPool>::instance();
    }

    sql::Connection* getConnection();
    bool releaseConnection(sql::Connection* conn);
private:
    MySQLConnectionPool(const string &host,
                        int port,
                        const string &datebaseName,
                        const string &user,
                        const string &password,
                        int poolSize = 4);
    ~MySQLConnectionPool();

    sql::Connection* createConnection();
private:
    string host_;
    int port_;
    string datebaseName_;
    string user_;
    string password_;
    int poolSize_;
    sql::Driver* driver_;
    deque<sql::Connection *> connections_;
    
private:
    MutexLock mutex_;
    Condition notEmpty_;
    Condition notFull_;
};

#endif // MYSQLCONNECTIONPOOL_H