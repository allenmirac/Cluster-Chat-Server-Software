#ifndef MYSQLCONNECTIONPOOL_H
#define MYSQLCONNECTIONPOOL_H

#include <string>
#include <deque>
#include <mysql-cppconn/jdbc/mysql_connection.h>
#include <mysql-cppconn/jdbc/mysql_driver.h>
#include <mysql-cppconn/jdbc/cppconn/exception.h>
#include <mysql-cppconn/jdbc/cppconn/driver.h>
#include <mysql-cppconn/jdbc/cppconn/connection.h>
#include <mysql-cppconn/jdbc/cppconn/resultset.h>
#include <mysql-cppconn/jdbc/cppconn/prepared_statement.h>
#include <mysql-cppconn/jdbc/cppconn/statement.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Condition.h>
#include <muduo/base/Singleton.h>
#include <muduo/base/Logging.h>

using namespace muduo;
using namespace std;

class MySQLConnectionPool : public noncopyable
{
public:
    static MySQLConnectionPool *getInstance();
    void initPool(const string &url = "tcp://127.0.0.1:3306",
                  const string &datebaseName = "chat",
                  const string &user = "root",
                  const string &password = "123456",
                  int poolSize = 8);

    sql::Connection *getConnection();
    bool releaseConnection(sql::Connection *conn);
    

private:
    MySQLConnectionPool() {}
    MySQLConnectionPool(const string &url,
                        const string &datebaseName,
                        const string &user,
                        const string &password,
                        int poolSize = 4);
    // 防止拷贝构造和赋值操作
    MySQLConnectionPool(const MySQLConnectionPool &) = delete;
    MySQLConnectionPool &operator=(const MySQLConnectionPool &) = delete;
    ~MySQLConnectionPool();

    void initConnection(int poolSize);
    sql::Connection *createConnection();
    void destoryConnection(sql::Connection *conn);
    void destoryConnectionPool();

private:
    string url_;
    string datebaseName_;
    string user_;
    string password_;
    int poolSize_;
    sql::Driver *driver_;
    deque<sql::Connection *> connections_;

private:
    static MySQLConnectionPool *instance_;
    static MutexLock mutex_;
    // static Condition notEmpty_;
    // static Condition notFull_;
};

#endif // MYSQLCONNECTIONPOOL_H