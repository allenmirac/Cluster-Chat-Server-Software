#include <iostream>
#include "mysqlconnectionpool.hpp"

using namespace std;

int main()
{
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    MySQLConnectionPool *mysqlPool1 = MySQLConnectionPool::getInstance();
    if (mysqlPool == mysqlPool1)
        cout << "instance is !!!!!" << endl;
    mysqlPool->initPool("tcp://127.0.0.1:3306", "chat", "root", "123456");

    sql::Connection *conn = mysqlPool->getConnection();
    if (nullptr == conn)
    {
        cout << "get False" << endl;
    }
    else
    {
        sql::Statement *state;
        sql::ResultSet *res;
        string sql = "select * from User";
        state = conn->createStatement();
        res = state->executeQuery(sql);
        while (res->next())
        {
            cout << "id = " << res->getInt(1) << endl;
            cout << "name = " << res->getString(2) << endl;
            cout << "password = " << res->getString(3) << endl;
            cout << "state = " << res->getString(4) << endl;
        }
        delete res;
        delete state;
        mysqlPool->releaseConnection(conn);
        // cout << "Get instance !!!!" <<endl;
    }
    return 0;
}