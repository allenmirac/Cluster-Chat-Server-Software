#include <iostream>
#include "include/server/mysqlconnectionpool/mysqlconnectionpool.hpp"

using namespace std;

int main(){
    MySQLConnectionPool& mysqlPool = MySQLConnectionPool::instance();
    sql::Connection* conn = mysqlPool.getConnection();
    if(nullptr == conn){
        cout<<"get False"<<endl;
    }
    return 0;    
}