#include "chatservice.hpp"
#include "public.hpp"
#include "user.hpp"
#include "usermodel.hpp"
#include <muduo/base/Logging.h>
#include <shared_mutex>

using namespace muduo;

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

    // supply local node id (can come from config/env)
    std::string localNodeId = "node-1"; // TODO: replace with real config
    redis_client_ = std::make_unique<RedisClient>();

    // register inbox callback: messages published to this node (node inbox or group channels) will be dispatched here
    redis_client_->setInboxCallback([this](const nlohmann::json& j) {
        // j should contain fields "to" (single recipient) or "groupid" for group messages
        if (j.contains("to")) {
            // single chat forwarded from remote node
            int to = j["to"];
            // dispatch on this node's event loop/context
            std::shared_lock<std::shared_mutex> lk(user_conn_mutex_);
            auto it = userConnMap_.find(to);
            if (it != userConnMap_.end() && it->second) {
                it->second->send(j.dump() + "\n");
            } else {
                // user not present locally -> store offline
                offlineMsgModel_.insert(to, j.dump());
                redis_client_->pushOfflineMessage(to, j);
            }
        } else if (j.contains("groupid")) {
            int groupid = j["groupid"];
            // find local members in groupMembers_[groupid]
            std::shared_lock<std::shared_mutex> lk2(group_mutex_);
            auto git = groupMembers_.find(groupid);
            if (git != groupMembers_.end()) {
                for (int uid : git->second) {
                    std::shared_lock<std::shared_mutex> lk3(user_conn_mutex_);
                    auto uit = userConnMap_.find(uid);
                    if (uit != userConnMap_.end() && uit->second) {
                        uit->second->send(j.dump() + "\n");
                    }
                }
            }
        }
    });
}

ChatService::~ChatService()
{
    msgHandlerMap_.clear();
    userConnMap_.clear();
    groupMembers_.clear();
    redis_client_.reset();
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
    // optionally mark all users offline in redis if graceful shutdown
}

// 处理登陆业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int id = js["id"];
    string pwd = js["password"];

    User userQuery;
    userQuery = userModel_.query(id);
    if (userQuery.getId() == id && userQuery.getPassword() == pwd)
    {
        if (redis_client_->isUserOnline(id)) {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errormsg"] = "该账号已经登陆，请重新输入账号";
            conn->send(response.dump());
            return;
        }

        // record local mapping (thread-safe)
        {
            std::unique_lock<std::shared_mutex> wlock(user_conn_mutex_);
            userConnMap_.insert({id, conn});
        }
        LOG_INFO << "ChatService::login, 用户 [" << userQuery.getName() << "] 登陆成功 on node " << redis_client_->getUserNode(id);

        // set presence in redis with local node id
        redis_client_->setUserOnline(id, conn->name());

        // prepare response
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 0;
        response["id"] = userQuery.getId();
        response["name"] = userQuery.getName();

        // pop offline msgs from redis
        auto offlineMsgs = redis_client_->popOfflineMessages(id);
        if (!offlineMsgs.empty()) {
            response["offlinemessage"] = offlineMsgs;
            // also remove persisted offline in DB if needed (your code may do that)
        }

        // friend list cache logic
        vector<std::string> tempFriendList;
        auto cachedFriends = redis_client_->get("friends:" + std::to_string(id));
        if (cachedFriends) {
            try {
                tempFriendList = nlohmann::json::parse(*cachedFriends).get<std::vector<std::string>>();
            } catch (...) { tempFriendList.clear(); }
        } else {
            vector<User> vFriendList = friendModel_.query(id);
            if (!vFriendList.empty())
            {
                for (User &user : vFriendList)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = redis_client_->isUserOnline(user.getId()) ? "online" : "offline";
                    tempFriendList.push_back(js.dump());
                }
                response["friends"] = tempFriendList;
                // cache friends list for N seconds
                redis_client_->set("friends:" + std::to_string(id), nlohmann::json(tempFriendList).dump(), std::chrono::hours(1));
            }
        }

        // group list: query DB, build local groupMembers_ map, and subscribe to group channels once per node
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
                    js["state"] = redis_client_->isUserOnline(user.getId()) ? "online" : "offline";
                    js["role"] = user.getRole();
                    vGroupUser.push_back(js.dump());
                }
                groupJson["users"] = vGroupUser;
                groupV.push_back(groupJson.dump());

                // update local groupMembers_ (so this node knows which local uids are in which groups)
                {
                    std::unique_lock<std::shared_mutex> glock(group_mutex_);
                    auto &memberSet = groupMembers_[group.getId()];
                    for (GroupUser &user : group.getGroupUsers()) {
                        // Only add local users into member set if they are local to this node (but we don't know that here)
                        // We add the currently logged-in user if it belongs to this group
                        if (user.getId() == id) {
                            memberSet.insert(id);
                        }
                    }
                }

                // ensure this node subscribes to group's channel once (if not already)
                redis_client_->subscribeGroupChannel(group.getId());
            }
            response["groups"] = groupV;
        }

        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errormsg"] = "账号或密码输入错误";
        conn->send(response.dump());
    }
}

void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);
    bool isInsert = userModel_.insert(user);
    if (isInsert)
    {
        redis_client_->hset("user_info:" + std::to_string(user.getId()), "name", name);
        redis_client_->expire("user_info:" + std::to_string(user.getId()), std::chrono::hours(24));
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

void ChatService::loginOut(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int id = js["id"];
    // remove local mapping
    {
        std::unique_lock<std::shared_mutex> wlock(user_conn_mutex_);
        userConnMap_.erase(id);
    }
    // mark offline in redis
    redis_client_->setUserOffline(id);
    // remove user from local groupMembers_ sets
    {
        std::unique_lock<std::shared_mutex> glock(group_mutex_);
        for (auto &kv : groupMembers_) {
            kv.second.erase(id);
            if (kv.second.empty()) {
                // if no local members of this group remain, ask redis client to unsubscribe
                redis_client_->unsubscribeGroupChannel(kv.first);
            }
        }
    }
    LOG_INFO << "ChatService::loginOut, 用户 [" << id << "] 登出";
}

void ChatService::clientQuitEcption(const TcpConnectionPtr &conn)
{
    User user;
    {
        std::unique_lock<std::shared_mutex> lock(user_conn_mutex_);
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
        redis_client_->setUserOffline(user.getId());
        // remove from groups
        std::unique_lock<std::shared_mutex> glock(group_mutex_);
        for (auto &kv : groupMembers_) {
            kv.second.erase(user.getId());
            if (kv.second.empty()) {
                redis_client_->unsubscribeGroupChannel(kv.first);
            }
        }
        LOG_ERROR << "ChatService::clientQuitEcption, 用户下线";
    }
}

// No Redis
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    json response;
    if (js["msg"].is_null() || js["msg"].get<string>().empty())
    {
        LOG_WARN << "ChatService::oneChat, 空消息被丢弃";
        return;
    }
    if (js.find("id") == js.end() || js.find("to") == js.end()) {
        LOG_ERROR << "ChatService::oneChat, 缺少必要字段";
        response["msgid"] = ONE_CHAT_MSG_ACK;
        response["errno"] = 1;
        response["errormsg"] = "Missing required fields";
        conn->send(response.dump());
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
        response["msgid"] = ONE_CHAT_MSG_ACK;
        response["errno"] = 1;
        response["errormsg"] = "No this friend";
        conn->send(response.dump());
        return;
    }
    {
        std::unique_lock<std::shared_mutex> lock(user_conn_mutex_);
        auto it = userConnMap_.find(toId);
        if (it != userConnMap_.end())
        {
            // 用户在线
            response["msgid"] = ONE_CHAT_MSG_ACK;
            response["errno"] = 0;
            response["msg"] = "MSG have sent";
            conn->send(response.dump());
            LOG_INFO << "ChatService::oneChat, 用户在线";
            it->second->send(js.dump());
            return;
        }
    }
    // 用户不在线
    offlineMsgModel_.insert(toId, js.dump());
    response["msgid"] = ONE_CHAT_MSG_ACK;
    response["errno"] = 0;
    response["msg"] = "Send offline MSG";
    conn->send(response.dump());
    LOG_INFO << "ChatService::oneChat, 用户不在线, 保存离线数据";
}

// void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp)
// {
//     json response;

//     if (js["msg"].is_null() || js["msg"].get<string>().empty())
//     {
//         LOG_WARN << "ChatService::oneChat, 空消息被丢弃";
//         return;
//     }
//     int id = js["id"];
//     int toId = js["to"];

//     // friend check (use cached friend list if available)
//     bool isFriend = false;
//     std::vector<std::string> tempFriendList;
//     auto cachedFriends = redis_client_->get("friends:" + std::to_string(id));
//     if (cachedFriends) {
//         try {
//             tempFriendList = nlohmann::json::parse(*cachedFriends).get<std::vector<std::string>>();
//             for (const auto& friendJson : tempFriendList) {
//                 json fj = nlohmann::json::parse(friendJson);
//                 if (fj["id"].get<int>() == toId) { isFriend = true; break; }
//             }
//         } catch (...) { tempFriendList.clear(); }
//     } else {
//         std::vector<User> vFriendList = friendModel_.query(id);
//         if (!vFriendList.empty()) {
//             for (User& user : vFriendList) {
//                 json fj;
//                 fj["id"] = user.getId();
//                 fj["name"] = user.getName();
//                 fj["state"] = redis_client_->isUserOnline(user.getId()) ? "online" : "offline";
//                 tempFriendList.push_back(fj.dump());
//                 if (user.getId() == toId) isFriend = true;
//             }
//             redis_client_->set("friends:" + std::to_string(id), nlohmann::json(tempFriendList).dump(), std::chrono::hours(1));
//         }

//     }
//     if (!isFriend) {
//         LOG_WARN << "ChatService::oneChat, 无此好友";
//         response["msgid"] = ONE_CHAT_MSG_ACK;
//         response["errno"] = 1;
//         response["errormsg"] = "No this friend";
//         conn->send(response.dump());
//         return;
//     }

//     // Route: check dest user's node
//     std::string destNode = redis_client_->getUserNode(toId);
//     if (destNode.empty()) {
//         // offline
//         redis_client_->pushOfflineMessage(toId, js);
//         offlineMsgModel_.insert(toId, js.dump());
//         response["msgid"] = ONE_CHAT_MSG_ACK;
//         response["errno"] = 0;
//         response["msg"] = "Save Offline MSG";
//         conn->send(response.dump());
//         LOG_INFO << "ChatService::oneChat, 用户 [" << toId << "] 离线，保存离线消息";
//         return;
//     }

//     if (destNode == redis_client_->getLocalNodeId()) {
//         // local delivery
//         std::shared_lock<std::shared_mutex> rl(user_conn_mutex_);
//         auto it = userConnMap_.find(toId);
//         if (it != userConnMap_.end() && it->second) {
//             response["msgid"] = ONE_CHAT_MSG_ACK;
//             response["errno"] = 0;
//             response["msg"] = "MSG Have Sent";
//             conn->send(response.dump());
//             it->second->send(js.dump() + "\n");
//             LOG_INFO << "ChatService::oneChat, 用户 [" << toId << "] 在线（本节点），消息已发送";
//         } else {
//             // not found locally though redis said this node: fallback to offline save
//             response["msgid"] = ONE_CHAT_MSG_ACK;
//             response["errno"] = 1;
//             response["errormsg"] = "In this node, but no connection";
//             conn->send(response.dump());
//             redis_client_->pushOfflineMessage(toId, js);
//             offlineMsgModel_.insert(toId, js.dump());
//             LOG_WARN << "ChatService::oneChat, 用户 [" << toId << "] 记录为本节点但未找到 connection，保存离线";
//         }
//     } else {
//         // remote node: publish to dest node inbox
//         response["msgid"] = ONE_CHAT_MSG_ACK;
//         response["errno"] = 0;
//         response["msg"] = "Not in this node, MSG Have publish";
//         conn->send(response.dump());
//         redis_client_->publishNodeInbox(destNode, js);
//         LOG_INFO << "ChatService::oneChat, 用户 [" << toId << "] 在节点 " << destNode << "，已 publish 到该节点 inbox";
//     }
// }

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"];
    int friendid = js["friendid"];

    friendModel_.insert(userid, friendid);
}

void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"];
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];

    Group group(-1, groupname, groupdesc);
    groupModel_.createGroup(group);
    groupModel_.joinGroup(userid, group.getId(), "creator");

    // local member addition and subscribe this group on node
    {
        std::unique_lock<std::shared_mutex> glock(group_mutex_);
        groupMembers_[group.getId()].insert(userid);
    }
    redis_client_->sadd("group_members:" + std::to_string(group.getId()), std::to_string(userid));
    redis_client_->expire("group_members:" + std::to_string(group.getId()), std::chrono::hours(1));
    redis_client_->subscribeGroupChannel(group.getId());
}

void ChatService::joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"];
    int groupid = js["groupid"];

    groupModel_.joinGroup(userid, groupid, "normal");

    {
        std::unique_lock<std::shared_mutex> glock(group_mutex_);
        groupMembers_[groupid].insert(userid);
    }

    redis_client_->sadd("group_members:" + std::to_string(groupid), std::to_string(userid));
    redis_client_->expire("group_members:" + std::to_string(groupid), std::chrono::hours(1));

    // ensure node subscribes to this group channel
    redis_client_->subscribeGroupChannel(groupid);
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    if (js["msg"].is_null() || js["msg"].get<string>().empty())
    {
        LOG_WARN << "ChatService::groupChat, 空群消息被丢弃";
        return;
    }

    int userid = js["id"];
    int groupid = js["groupid"];

    // If we don't have local membership info, load from DB and update set
    {
        std::shared_lock<std::shared_mutex> glock(group_mutex_);
        if (groupMembers_.find(groupid) == groupMembers_.end()) {
            glock.unlock();
            // fallback: query DB
            auto groupUsers = groupModel_.queryGroupUsers(userid, groupid);
            std::unique_lock<std::shared_mutex> glock2(group_mutex_);
            for (int id : groupUsers) {
                groupMembers_[groupid].insert(id);
            }
            // ensure subscription
            redis_client_->subscribeGroupChannel(groupid);
        }
    }

    // publish group message: each node subscribed to this group will receive and distribute locally
    redis_client_->publishGroupMessage(groupid, js);
    LOG_INFO << "ChatService::groupChat, 群 [" << groupid << "] 消息已发布";
}
