#include "chatservice.hpp"
#include "public.hpp"
#include "user.hpp"
#include "usermodel.hpp"
#include <muduo/base/Logging.h>

ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    msgHandlerMap_.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    msgHandlerMap_.insert({LOGIN_OUT_MSG, std::bind(&ChatService::loginOut, this, _1, _2, _3)});
    msgHandlerMap_.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    msgHandlerMap_.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    msgHandlerMap_.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    msgHandlerMap_.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    msgHandlerMap_.insert({JOIN_GROUP_MSG, std::bind(&ChatService::joinGroup, this, _1, _2, _3)});
    msgHandlerMap_.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
}

ChatService::~ChatService()
{
    msgHandlerMap_.clear();
}

MsgHandler ChatService::getHandler(int msgid)
{
    auto it = msgHandlerMap_.find(msgid);
    if (it == msgHandlerMap_.end())
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "Msgid is " << msgid << " can not find handler!";
        };
    }
    else
    {
        return msgHandlerMap_[msgid];
    }
}

void ChatService::reset()
{
    userModel_.resetState();
}

// 处理登陆业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"];
    string pwd = js["password"];

    User userQuery;
    userQuery = userModel_.query(id);
    if (userQuery.getId() == id && userQuery.getPassword() == pwd)
    {
        if (userQuery.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errormsg"] = "该账号已经登陆，请重新输入账号";
            conn->send(response.dump());
        }
        else
        {
            // 连接成功
            {
                // 锁的粒度尽可能小
                lock_guard<mutex> lock(mutex_);
                userConnMap_.insert({id, conn});
                LOG_INFO << "ChatService::login, 用户 [" << userQuery.getName() << "] 登陆成功";
            }
            userQuery.setState("online");
            userModel_.updateState(userQuery);
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = userQuery.getId();
            response["name"] = userQuery.getName();

            //
            vector<string> vOfflineMsg = offlineMsgModel_.query(id);
            if (!vOfflineMsg.empty())
            {
                response["offlinemessage"] = vOfflineMsg;
                // LOG_INFO << "v:" << v[0];
                offlineMsgModel_.remove(id);
            }

            vector<User> vFriendList = friendModel_.query(id);
            if (!vFriendList.empty())
            {
                vector<string> tempFriendList;
                for (User &user : vFriendList)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    tempFriendList.push_back(js.dump());
                }
                response["friends"] = tempFriendList;
            }

            vector<Group> vGroupList = groupModel_.queryGroups(id);
            if(!vGroupList.empty())
            {
                vector<string> groupV;
                for(Group &group : vGroupList)
                {
                    json groupJson;
                    groupJson["id"] = group.getId();
                    groupJson["groupname"] = group.getName();
                    groupJson["groupdesc"] = group.getDesc();
                    vector<string> vGroupUser;
                    for(GroupUser &user : group.getGroupUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        vGroupUser.push_back(js.dump());
                    }
                    groupJson["users"] = vGroupUser;
                    groupV.push_back(groupJson.dump());
                }
                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errormsg"] = "账号或密码输入错误";
        conn->send(response.dump());
    }
    // LOG_INFO << "login 回调处理";
}

void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);
    bool isInsert = userModel_.insert(user);
    if (isInsert)
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

void ChatService::loginOut(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"];
    string name = js["name"];
    {
        lock_guard<mutex> lock(mutex_);
        auto it = userConnMap_.find(id);
        if(it!=userConnMap_.end()){
            userConnMap_.erase(it);
        }
    }
    User user(id, name, "", "offline");
    userModel_.updateState(user);
}

void ChatService::clientQuitEcption(const TcpConnectionPtr &conn)
{
    User user;
    {

        lock_guard<mutex> lock(mutex_);
        for (auto it = userConnMap_.begin(); it != userConnMap_.end(); it++)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                userConnMap_.erase(it);
                break;
            }
        }
    }
    if (user.getId() != -1)
    {
        user.setState("offline");
        userModel_.updateState(user);
        LOG_ERROR << "ChatService::clientQuitEcption, 用户下线";
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    if (js["msg"].is_null() || js["msg"].get<string>().empty())
    {
        LOG_WARN << "ChatService::oneChat, 空消息被丢弃";
        return;
    }
    int id = js["id"];
    int toId = js["to"];
    bool isFriend = false;
    vector<User> vFriendList = friendModel_.query(id);
    if (!vFriendList.empty())
    {
        vector<string> tempFriendList;
        for (User &user : vFriendList)
        {
            int tempId = user.getId();
            if(tempId == toId) {
                isFriend = true;
                break;
            }
        }
    }
    if (isFriend == false) {
        LOG_WARN << "ChatService::oneChat, 无此好友";
        return;
    }
    {
        lock_guard<mutex> lock(mutex_);
        auto it = userConnMap_.find(toId);
        if (it != userConnMap_.end())
        {
            // 用户在线
            LOG_INFO << "ChatService::oneChat, 用户在线";
            it->second->send(js.dump());
            return;
        }
    }
    // 用户不在线
    offlineMsgModel_.insert(toId, js.dump());
    LOG_INFO << "ChatService::oneChat, 用户不在线, 保存离线数据";
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];
    int friendid = js["friendid"];

    friendModel_.insert(userid, friendid);
}

void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];

    Group group(-1, groupname, groupdesc);
    groupModel_.createGroup(group);
    groupModel_.joinGroup(userid, group.getId(), "creator");
}

void ChatService::joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];
    int groupid = js["groupid"];

    groupModel_.joinGroup(userid, groupid, "normal");
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    if (js["msg"].is_null() || js["msg"].get<string>().empty())
    {
        LOG_WARN << "ChatService::groupChat, 空群消息被丢弃";
        return;
    }

    int userid = js["id"];
    int groupid = js["groupid"];
    vector<int> groupUsers = groupModel_.queryGroupUsers(userid, groupid);
    
    lock_guard<mutex> lock(mutex_);
    for (int id : groupUsers)
    {
        
        auto it = userConnMap_.find(id);
        if (it != userConnMap_.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            offlineMsgModel_.insert(userid, js.dump());
        }
    }
}