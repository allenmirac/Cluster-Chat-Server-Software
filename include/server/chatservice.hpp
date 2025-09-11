#ifndef CHATSERVICE_HPP
#define CHATSERVICE_HPP

#include "redisclient.hpp"
#include "usermodel.hpp"
#include "offlinemessage.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp>
#include <functional>
#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <vector>

using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;
using MsgHandler = std::function<void(const TcpConnectionPtr&, json&, Timestamp)>;

class ChatService {
public:
    static ChatService* instance();
    MsgHandler getHandler(int msgid);
    void reset();
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void loginOut(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void clientQuitEcption(const TcpConnectionPtr& conn);
    void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void joinGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);

private:
    ChatService();
    ~ChatService();

    std::unordered_map<int, MsgHandler> msgHandlerMap_;

    std::unordered_map<int, TcpConnectionPtr> userConnMap_;
    std::shared_mutex user_conn_mutex_; // 保护 userConnMap_ 的读写锁

    std::unordered_map<int, std::set<int>> groupMembers_;
    std::shared_mutex group_mutex_; // 保护 groupMembers_ 的读写锁

    UserModel userModel_;
    OfflineMessage offlineMsgModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;

    std::unique_ptr<RedisClient> redis_client_;
};

#endif // CHATSERVICE_HPP