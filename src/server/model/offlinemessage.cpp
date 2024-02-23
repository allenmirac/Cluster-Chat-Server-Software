#include <cppconn/prepared_statement.h>
#include <vector>
#include "mysqlconnectionpool.hpp"
#include "offlinemessage.hpp"

bool OfflineMessage::insert(int userId, string message)
{
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    sql::Connection *conn = mysqlPool->getConnection();
    // 使用预处理语句，防止 sql 注入
    try
    {
        sql::PreparedStatement *pstmt;
        pstmt = conn->prepareStatement("insert into OfflineMessage(userid, message) values(?, ?)");
        pstmt->setInt(1, userId);
        pstmt->setString(2, message);
        pstmt->executeUpdate();

        delete pstmt;
        mysqlPool->releaseConnection(conn);
        return true;
    }
    catch (sql::SQLException &e)
    {
        LOG_ERROR << "OfflineMessage::insert, SQL Exception: " << e.what();
    }
    return false;
}
bool OfflineMessage::remove(int userId)
{
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    sql::Connection *conn = mysqlPool->getConnection();
    // 使用预处理语句，防止 sql 注入
    try
    {
        sql::PreparedStatement *pstmt;
        pstmt = conn->prepareStatement("DELETE FROM OfflineMessage where userid=?");
        pstmt->setInt(1, userId);
        pstmt->executeUpdate();

        delete pstmt;
        mysqlPool->releaseConnection(conn);
        return true;
    }
    catch (sql::SQLException &e)
    {
        LOG_ERROR << "OfflineMessage::insert, SQL Exception: " << e.what();
    }
    return false;
}
vector<string> OfflineMessage::query(int userId)
{
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    sql::Connection *conn = mysqlPool->getConnection();
    vector<string> v;
    sql::Statement *stmt;
    sql::ResultSet *res;
    try
    {
        stmt = conn->createStatement();
        string sql = "select userid, message from OfflineMessage where userid=" + to_string(userId);
        res = stmt->executeQuery(sql);
        string queryRes;
        while(res->next()){
            queryRes = res->getString(2);
            v.push_back(queryRes);
        }
        delete res;
        delete stmt;
        mysqlPool->releaseConnection(conn);
        if(!v.empty())
        {
            // LOG_INFO << "v:" << v[0];
            return v;
        }
        else
        {
            LOG_ERROR << "UserModel::query, 用户 [" << userId << "] 没有离线消息";
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "OfflineMessage::query error" << e.what() << '\n';
    }
    return vector<string>();
}