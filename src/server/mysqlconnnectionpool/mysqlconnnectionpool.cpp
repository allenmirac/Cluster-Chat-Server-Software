#include "mysqlconnectionpool.hpp"

MySQLConnectionPool::MySQLConnectionPool(const string &host,
                                         int port,
                                         const string &datebaseName,
                                         const string &user,
                                         const string &password,
                                         int poolSize = 4)
    : host_(host), port_(port), datebaseName_(datebaseName), user_(user), password_(password), poolSize_(poolSize),
      mutex_(), notEmpty_(mutex_), notFull_(mutex_)
{
    this->driver_ = sql::mysql::get_mysql_driver_instance();
    for (int i = 0; i < poolSize; i++)
    {
        sql::Connection *conn = nullptr;
        conn = this->createConnection();
        if (conn == nullptr)
        {
            LOG_ERROR << "MySQLConntcionPool createConnection error!";
        }
        connections_.push_back(conn);
    }
}

sql::Connection *MySQLConnectionPool::getConnection()
{
    sql::Connection* conn=nullptr;
    if(0 == connections_.size()){
        return nullptr;
    }
    MutexLockGuard lock(mutex_);
    conn = connections_.front();
    connections_.pop_front();

    return conn;
}

bool MySQLConnectionPool::releaseConnection(sql::Connection* conn)
{
    if ( conn == nullptr) return false;
    MutexLockGuard lock(mutex_);
    connections_.push_back(conn);
    return true;
}

sql::Connection *MySQLConnectionPool::createConnection()
{
    sql::Connection* conn=nullptr;
    string url = host_ + ":" + to_string(port_); /*eg:"tcp://127.0.0.1:3306"*/
    conn = driver_->connect(url, user_, password_);
    conn->setSchema(datebaseName_);

    return conn;
}