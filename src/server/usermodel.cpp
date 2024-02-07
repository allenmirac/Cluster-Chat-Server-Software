#include "../../include/server/usermodel.hpp"
#include "../../include/server/mysqlconnectionpool/mysqlconnectionpool.hpp"
#include <iostream>
#include <cppconn/prepared_statement.h>
using namespace std;

bool UserModel::insert(User &user)
{
    // char sql[1024] = {0};
    // sprintf(sql, "insert into User(name, password, state) values('%s', '%s', '%s')", user.getName(), user.getPassword(), user.getState());
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    mysqlPool->initPool();
    sql::Connection *conn = mysqlPool->getConnection();
    // 使用预处理语句，防止 sql 注入
    try
    {
        sql::PreparedStatement *pstmt;
        pstmt = conn->prepareStatement("insert into User(name, password, state) values(?, ?, ?)");
        pstmt->setString(1, user.getName());
        pstmt->setString(2, user.getPassword());
        pstmt->setString(3, user.getState());
        pstmt->executeUpdate();

        delete pstmt;
        mysqlPool->releaseConnection(conn);
        return true;
    }
    catch (sql::SQLException &e)
    {
        LOG_ERROR << "UserModel insert SQL Exception: " << e.what();
    }
    return false;
}

User UserModel::query(int id)
{
    User user;
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    mysqlPool->initPool();
    sql::Connection *conn = mysqlPool->getConnection();
    sql::Statement *stmt;
    sql::ResultSet *res;
    try
    {
        stmt = conn->createStatement();
        string sql = "select * from User where id=" + to_string(id);
        res = stmt->executeQuery(sql);
        // LOG_ERROR << "res->rowsCount(): " << res->rowsCount();
        if (res->next())
        {
            user.setId(res->getInt(1));
            user.setName(res->getString(2));
            user.setPassword(res->getString(3));
            user.setState(res->getString(4));
        }
        else
        {
            LOG_ERROR << "UserModel::query, No data found for id: " << id;
        }
        delete stmt;
        delete res;
        mysqlPool->releaseConnection(conn);
        return user;
    }
    catch (sql::SQLException &e)
    {
        LOG_ERROR << "UserModel::query, SQL Exception: " << e.what();
    }
    return User();
}