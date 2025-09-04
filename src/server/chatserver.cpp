#include "chatserver.hpp"
#include "chatservice.hpp"
#include <muduo/base/Logging.h>

#include "json.hpp"
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : server_(loop, listenAddr, nameArg), loop_(loop)
{
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    server_.setThreadNum(4);
}

void ChatServer::start()
{
    server_.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        ChatService::instance()->clientQuitEcption(conn);
        conn->shutdown();
    }
    else
    {
        LOG_INFO << "ChatServer::onConnection, 有一个新连接";
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    // 接收数据的反序列化
    try
    {
        json js = json::parse(buf);
        // handler 达到的目的：完全解耦网络模块的代码和业务模块的代码
        // 通过js["msgid"] => 获取业务的 handler => conn js time
        auto handler = ChatService::instance()->getHandler(js["msgid"].get<int>());
        handler(conn, js, time);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR <<"ChatServer::onMessage, JSON parse error: " << e.what();
    }
}
