#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <mutex>
#include <muduo/base/Condition.h>
#include <unordered_map>
#include <functional>
#include "usermodel.hpp"
#include "offlinemessage.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "json.hpp"
using json = nlohmann::json;

// 将其当成函数指针来使用
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

class ChatService
{
public:
    static ChatService *instance();
    // 处理登陆业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp);

    MsgHandler getHandler(int msgid);

    void clientQuitEcption(const TcpConnectionPtr &conn);

    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
private:
    ChatService();
    ~ChatService();
    ChatService(const ChatService &) = delete;
    ChatService &operator=(const ChatService &) = delete;
private:
    // 存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> msgHandlerMap_;

    // 存储连接信息
    unordered_map<int, TcpConnectionPtr> userConnMap_;


    // 连接锁
    mutex mutex_;

    UserModel userModel_;
    OfflineMessage offlineMsgModel_;
};

#endif // CHATSERVICE_H