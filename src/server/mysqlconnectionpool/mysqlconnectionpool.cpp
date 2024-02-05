#include "mysqlconnectionpool.hpp"

MySQLConnectionPool *MySQLConnectionPool::instance_ = nullptr;
MutexLock MySQLConnectionPool::mutex_;
// Condition notEmpty_;
// Condition notFull_;

MySQLConnectionPool *MySQLConnectionPool::getInstance()
{
    if (nullptr == instance_)
    {
        MutexLockGuard lock(mutex_);
        if (nullptr == instance_)
        {
            instance_ = new MySQLConnectionPool();
        }
    }
    return instance_;
}

void MySQLConnectionPool::initPool(const string &url,
                                   const string &datebaseName,
                                   const string &user,
                                   const string &password,
                                   int poolSize)
{
    url_ = url;
    datebaseName_ = datebaseName;
    user_ = user;
    password_ = password;
    poolSize_ = poolSize;

    try
    {
        driver_ = sql::mysql::get_mysql_driver_instance();
    }
    catch (sql::SQLException &e)
    {
        // LOG_ERROR << "MySQLConntcionPool initPool error!";
        LOG_INFO << "MySQLConntcionPool initPool error!";

    }
    catch (runtime_error &e)
    {
        // LOG_ERROR << "MySQLConntcionPool runtime error!";
        LOG_INFO << "MySQLConntcionPool runtime error!";
    }
    initConnection(poolSize);
}

void MySQLConnectionPool::initConnection(int poolSize)
{
    for (int i = 0; i < poolSize; i++)
    {
        MutexLockGuard lock(mutex_);
        sql::Connection *conn = nullptr;
        conn = this->createConnection();
        if (conn == nullptr)
        {
            // LOG_ERROR << "MySQLConntcionPool createConnection error!";
            LOG_INFO << "MySQLConntcionPool createConnection error!";

        }
        connections_.push_back(conn);
    }
}

sql::Connection *MySQLConnectionPool::getConnection()
{
    sql::Connection *conn = nullptr;
    if (0 == connections_.size())
    {
        return nullptr;
    }
    MutexLockGuard lock(mutex_);
    conn = connections_.front();
    connections_.pop_front();

    return conn;
}

bool MySQLConnectionPool::releaseConnection(sql::Connection *conn)
{
    if (conn == nullptr)
        return false;
    MutexLockGuard lock(mutex_);
    connections_.push_back(conn);
    return true;
}

MySQLConnectionPool::MySQLConnectionPool(const string &url,
                                         const string &datebaseName,
                                         const string &user,
                                         const string &password,
                                         int poolSize)
    : url_(url), datebaseName_(datebaseName), user_(user), password_(password), poolSize_(poolSize)
{
    this->driver_ = sql::mysql::get_mysql_driver_instance();
    for (int i = 0; i < poolSize; i++)
    {
        sql::Connection *conn = nullptr;
        conn = this->createConnection();
        if (conn == nullptr)
        {
            // LOG_ERROR << "MySQLConntcionPool createConnection error!";
            LOG_INFO << "MySQLConntcionPool createConnection error!";
        }
        connections_.push_back(conn);
    }
}

MySQLConnectionPool::~MySQLConnectionPool(){
    destoryConnectionPool();
}

sql::Connection *MySQLConnectionPool::createConnection()
{
    sql::Connection *conn = nullptr;
    conn = driver_->connect(url_, user_, password_);
    conn->setSchema(datebaseName_);

    return conn;
}

void MySQLConnectionPool::destoryConnection(sql::Connection *conn)
{
    if (conn)
    {
        try
        {
            conn->close();
        }
        catch (sql::SQLException &e)
        {
            // LOG_ERROR << "MySQLConntcionPool destoryConnection error!" << e.what();
            LOG_INFO << "MySQLConntcionPool destoryConnection error!" << e.what();

        }
        catch (exception &e)
        {
            // LOG_ERROR << e.what();
            LOG_INFO << e.what();
        }
        delete conn;
    }
}
void MySQLConnectionPool::destoryConnectionPool()
{
    MutexLockGuard lock(mutex_);
    for (auto it = connections_.begin(); it != connections_.end(); ++it)
    {
        destoryConnection(*it);
    }
    connections_.clear();
}