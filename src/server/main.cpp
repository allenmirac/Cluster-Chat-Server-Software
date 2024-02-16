#include <iostream>
#include "chatserver.hpp"
#include "../../include/server/mysqlconnectionpool/mysqlconnectionpool.hpp"

using namespace std;

int main()
{
    EventLoop loop;
    InetAddress addr("127.0.0.1", 2222);
    ChatServer server(&loop, addr, "ChatServer");
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    mysqlPool->initPool();

    server.start();
    loop.loop();

    return 0;
}