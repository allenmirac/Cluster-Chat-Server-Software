#include "friendmodel.hpp"
#include "mysqlconnectionpool.hpp"
#include <cppconn/prepared_statement.h>

void FriendModel::insert(int userid, int frindid)
{
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    sql::Connection *conn = mysqlPool->getConnection();
    try
    {
        sql::PreparedStatement *pstmt;
        pstmt = conn->prepareStatement("insert into Friend(userid, friendid) values(?, ?)");
        pstmt->setInt(1, userid);
        pstmt->setInt(2, frindid);
        pstmt->executeUpdate();

        delete pstmt;
        mysqlPool->releaseConnection(conn);
    }
    catch (sql::SQLException &e)
    {
        LOG_ERROR << "FriendModel::insert, SQL Exception: " << e.what();
    }
}

vector<User> FriendModel::query(int userid)
{
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    sql::Connection *conn = mysqlPool->getConnection();
    sql::Statement *stmt;
    sql::ResultSet *res;
    vector<User> v;
    try
    {
        stmt = conn->createStatement();
        string sql = "SELECT a.id, a.name, a.state FROM User as a JOIN Friend as b ON a.id = b.friendid where b.userid="+to_string(userid);
        res = stmt->executeQuery(sql);
        while(res->next()){
            User user;
            user.setId(res->getInt(1));
            user.setName(res->getString(2));
            user.setState(res->getString(3));
            v.push_back(user);
        }
        delete res;
        delete stmt;
        
        return v;
        // select a.id, a.name, a,state from User as a join Friend as b on a.id=b.userid;
    }
    catch(const sql::SQLException& e)
    {
        LOG_ERROR << "UserModel::resetState, SQL Exception: " << e.what();
    }
    return vector<User>();
}