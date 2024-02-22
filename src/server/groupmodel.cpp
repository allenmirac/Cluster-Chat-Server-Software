#include "groupmodel.hpp"
#include "mysqlconnectionpool.hpp"

void GroupModel::createGroup(Group &group)
{
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    sql::Connection *conn = mysqlPool->getConnection();
    sql::ResultSet *res;
    sql::PreparedStatement *pstmt;
    try
    {
        string sql = "insert into AllGroup(groupname, groupdesc) values(?, ?)";
        pstmt = conn->prepareStatement(sql);
        pstmt->setString(1, group.getName());
        pstmt->setString(2, group.getDesc());
        pstmt->executeUpdate();
        // res = pstmt->getGeneratedKeys();
        sql = "SELECT LAST_INSERT_ID() AS last_id";
        pstmt = conn->prepareStatement(sql);
        res = pstmt->executeQuery();
        if (res->next())
        {
            group.setId(res->getInt(1));
        }
        delete pstmt;
        delete res;
        mysqlPool->releaseConnection(conn);
    }
    catch (sql::SQLException &e)
    {
        LOG_ERROR << "GroupModel::createGroup, SQL Exception: " << e.what();
    }
}

void GroupModel::joinGroup(int userid, int groupid, string role)
{
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    sql::Connection *conn = mysqlPool->getConnection();
    try
    {
        sql::PreparedStatement *pstmt;
        pstmt = conn->prepareStatement("insert into GroupUser(groupid, userid, grouprole) values(?, ?, ?)");
        pstmt->setInt(1, groupid);
        pstmt->setInt(2, userid);
        pstmt->setString(3, role);
        pstmt->executeUpdate();

        delete pstmt;
        mysqlPool->releaseConnection(conn);
    }
    catch (sql::SQLException &e)
    {
        LOG_ERROR << "GroupModel::joinGroup, SQL Exception: " << e.what();
    }
}

vector<Group> GroupModel::queryGroups(int userid)
{
    vector<Group> vGroup;
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    sql::Connection *conn = mysqlPool->getConnection();
    sql::Statement *stmt;
    sql::ResultSet *res;
    try
    {
        stmt = conn->createStatement();
        string sql = "select a.id, a.groupname, a.groupdesc from AllGroup as a join GroupUser as b on a.id=b.groupid where b.userid=" + to_string(userid);
        res = stmt->executeQuery(sql);
        while (res->next())
        {
            Group group;
            group.setId(res->getInt(1));
            group.setName(res->getString(2));
            group.setDesc(res->getString(3));
            vGroup.push_back(group);
        }

        
        for (Group it : vGroup)
        {
            vector<GroupUser> vGroupUsers;
            GroupUser groupUser;
            sql = "select a.id, a.name, a.state, b.grouprole from User as a join GroupUser as b on a.id=b.userid where b.groupid=" + to_string(it.getId());
            res = stmt->executeQuery(sql);
            while(res->next())
            {
                groupUser.setId(res->getInt(1));
                groupUser.setName(res->getString(2));
                groupUser.setState(res->getString(3));
                groupUser.setRole(res->getString(4));
                vGroupUsers.push_back(groupUser);
            }
            it.setUsers(vGroupUsers);
        }

        delete stmt;
        delete res;
        mysqlPool->releaseConnection(conn);
        return vGroup;
    }
    catch (sql::SQLException &e)
    {
        LOG_ERROR << "GroupModel::queryGroups, SQL Exception: " << e.what();
    }
    return vector<Group>();
}

vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    vector<int> vGroupUser;
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    sql::Connection *conn = mysqlPool->getConnection();
    sql::Statement *stmt;
    sql::ResultSet *res;
    try
    {
        stmt = conn->createStatement();
        string sql = "select userid from GroupUser where groupid=" + to_string(groupid) + " and userid<>" + to_string(userid);
        // LOG_INFO << sql;
        res = stmt->executeQuery(sql);
        
        while (res->next())
        {
            int userid;
            userid = res->getInt(1);
            vGroupUser.push_back(userid);
        }
        delete stmt;
        delete res;
        mysqlPool->releaseConnection(conn);
        return vGroupUser;
    }
    catch (sql::SQLException &e)
    {
        LOG_ERROR << "GroupModel::queryGroupUsers, SQL Exception: " << e.what();
    }
    return vector<int>();
}