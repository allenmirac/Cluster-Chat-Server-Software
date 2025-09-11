#include <iostream>
#include <signal.h>
#include "chatserver.hpp"
#include "chatservice.hpp"
#include "mysqlconnectionpool.hpp"
#include "redisconnectionpool.hpp"

using namespace std;

// 服务器异常，业务重置
void statusResetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main()
{
    signal(SIGINT, statusResetHandler);

    EventLoop loop;
    InetAddress addr("127.0.0.1", 2222);
    ChatServer server(&loop, addr, "ChatServer");
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    RedisConnectionPool *redisPool = RedisConnectionPool::getInstance();
    mysqlPool->initPool();
    redisPool->init();
    ChatService::instance()->reset();

    server.start();
    loop.loop();
    // loop.runInLoop();

    return 0;
}