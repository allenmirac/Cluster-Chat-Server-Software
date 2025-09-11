#include <iostream>
#include <unordered_map>
#include <functional>
#include <vector>
#include <chrono>
#include <ctime>
#include <thread>
#include "user.hpp"
#include "group.hpp"
#include "public.hpp"
using namespace std;

#include "json.hpp"
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

bool isMainMenuRunning = false;
atomic_bool isLoginSuccess{false};
sem_t rwSem; // 读写的信号通知
User currentUser;
vector<User> currentUserFriendList;
vector<Group> currentUserGroupList;

void error_if(bool condition, const char *errmsg);

void mainmenu(int);
void readTaskHandler(int clientfd);
void showCurrentUserData();
string getCurrentTime();

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << "command invalid ! example:./ChatClient 127.1 2222" << endl;
        exit(-1);
    }
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "Client socket create error!!" << endl;
        exit(-1);
    }

    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET; // 地址族，设置为IPv4
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);
    cerr << "Port:" << port << " ip:" << ip << endl;
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "Client connect server error!!" << endl;
        close(clientfd);
        exit(-1);
    }

    sem_init(&rwSem, 0, 0);
    thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    for (;;)
    {
        cout << "=======================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "=======================" << endl;
        cout << "Your choice:";
        int choice = 0;
        cin >> choice;
        cin.get();

        switch (choice)
        {
        case 1:
        {
            int id = 0;
            char pwd[20] = {0};
            cout << "Userid:";
            cin >> id;
            cin.get();
            cout << "Password:";
            cin.getline(pwd, 20);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            isLoginSuccess = false;
            int len = send(clientfd, request.c_str(), request.size(), 0);
            if (-1 == len)
            {
                cerr << "Send login msg error!!" << request << endl;
            }
            sem_wait(&rwSem);
            if (isLoginSuccess)
            {
                isMainMenuRunning = true;
                mainmenu(clientfd);
            }
        }
        break;
        case 2: // register业务
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }

            sem_wait(&rwSem); // 等待信号量，子线程处理完注册消息会通知
        }
        break;
        case 3: // quit业务
            close(clientfd);
            sem_destroy(&rwSem);
            exit(0);

        default:
            cerr << "Invalid input!!" << endl;
            break;
        }
    }
    // cout << "Hello world" << endl;
    // mainmenu(0);
    return 0;
}

void doRegResponse(json &responsejs)
{
    if (0 != responsejs["errno"])
    {
        cerr << "name is already exist, register error!" << endl;
    }
    else
    {
        cout << "name register success, userid is " << responsejs["id"] << ", do not forget it" << endl;
    }
}

void doLoginResponse(json &responsejs)
{
    if (0 != responsejs["errno"])
    {
        cerr << responsejs["errormsg"] << endl;
        isLoginSuccess = false;
    }
    else
    {
        currentUser.setId(responsejs["id"]);
        // currentUser.setName(responsejs["name"]);
        if (responsejs.contains("friends"))
        {
            currentUserFriendList.clear();
            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["id"]);
                user.setName(js["name"]);
                user.setState(js["state"]);
                currentUserFriendList.push_back(user);
            }
        }

        if (responsejs.contains("groups"))
        {
            currentUserGroupList.clear();
            vector<string> vec = responsejs["groups"];
            for (string &groupstr : vec)
            {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"]);
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                vector<string> vec2 = grpjs["users"];
                vector<GroupUser> groupVec;
                for (string &userstr : vec2)
                {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setId(js["id"]);
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    groupVec.push_back(user);
                }
                group.setUsers(groupVec);
                currentUserGroupList.push_back(group);
            }
        }
        showCurrentUserData();

        if (responsejs.contains("offlinemessage"))
        {
            vector<string> vec = responsejs["offlinemessage"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                // time + [id] + name + " said: " + xxx
                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                {
                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
                else
                {
                    cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
            }
        }
        isLoginSuccess = true;
    }
}

void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (len == -1 || len == 0)
        {
            close(clientfd);
            exit(-1);
        }

        json js = json::parse(buffer);
        int msgType = js["msgid"];
        switch (msgType)
        {
        case ONE_CHAT_MSG:
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            break;
        case GROUP_CHAT_MSG:
            cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            break;
        case LOGIN_MSG_ACK:
            doLoginResponse(js);
            sem_post(&rwSem);
            break;
        case REG_MSG_ACK:
            doRegResponse(js);
            sem_post(&rwSem);
            break;
        default:
            break;
        }
    }
}

void showCurrentUserData()
{
    cout << "=============Current User=================" << endl;
    cout << "Current User id: [ " << currentUser.getId() << " ] name: [ " << currentUser.getName() << " ]" << endl;
    cout << "-------------Friend List------------------" << endl;
    if (!currentUserFriendList.empty())
    {
        for (User &user : currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "-------------Group List-------------------" << endl;
    if (!currentUserGroupList.empty())
    {
        for (Group &group : currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getGroupUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    cout << "===========================================" << endl;
}

void help(int fd = 0, string str = "");
void chat(int, string);
void addFriend(int, string);
void createGroup(int, string);
void joinGroup(int, string);
void groupChat(int, string);
void loginout(int, string);

unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addFriend", "添加好友，格式addFriend:friendid"},
    {"createGroup", "创建群聊，格式createGroup:groupName:groupDesc"},
    {"joinGroup", "加入群聊，格式joinGroup:groupid"},
    {"groupChat", "群聊，格式groupChat:groupid:message"},
    {"loginout", "注销，格式loginout"}};

unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addFriend", addFriend},
    {"createGroup", createGroup},
    {"joinGroup", joinGroup},
    {"groupChat", groupChat},
    {"loginout", loginout}};

void mainmenu(int clientFd)
{
    help();
    char buffer[1024];
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandBuffer(buffer);
        string command;
        int idx = commandBuffer.find(":"); // 第一次出现 : 的位置
        if (-1 == idx)
        {
            command = commandBuffer;
        }
        else
        {
            command = commandBuffer.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "输入了不合法的命令" << endl;
            continue;
        }
        it->second(clientFd, commandBuffer.substr(idx + 1, commandBuffer.size() - idx));
    }
}

void help(int, string)
{
    cout << "Show command list >>> " << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << ":" << p.second << endl;
    }
    cout << endl;
}

void chat(int clientfd, string str)
{
    int idx = str.find(":");
    error_if(-1 == idx, "chat command invalid");
    if(-1 == idx){
        return ;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
    if (message.empty())
    {
        cerr << "私聊消息不能为空！" << endl;
        return;
    }

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = currentUser.getId();
    js["name"] = currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), buf.size(), 0); // 向客户端套接字发送数据
    error_if(-1 == len, "send chat msg error");
}

void addFriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = currentUser.getId();
    js["friendid"] = friendid;
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), buf.size(), 0);
    error_if(-1 == len, "send addFriend msg error");
}

void createGroup(int clientfd, string str)
{
    int idx = str.find(":");
    error_if(-1 == idx, "creategroup command invalid!!!");
    if(-1 == idx){
        return ;
    }

    string groupName = str.substr(0, idx);
    string groupDesc = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = currentUser.getId();
    js["groupname"] = groupName;
    js["groupdesc"] = groupDesc;
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), buf.size(), 0);
    error_if(-1 == len, "createGroup msg error!!!");
}

void joinGroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = JOIN_GROUP_MSG;
    js["id"] = currentUser.getId();
    js["groupid"] = groupid;
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), buf.size(), 0);
    error_if(-1 == len, "joinGroup msg error !!!");
}

void groupChat(int clientfd, string str)
{
    int idx = str.find(":");
    error_if(-1 == idx, "groupChat command invalid!!");
    if(-1 == idx){
        return ;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string groupMsg = str.substr(idx + 1, str.size() - idx);

    if (groupMsg.empty())
    {
        cerr << "群聊消息不能为空！" << endl;
        return;
    }

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = currentUser.getId();
    js["name"] = currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = groupMsg;
    js["time"] = getCurrentTime();
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), buf.size(), 0);
    error_if(-1 == len, "groupChat msg error!!");
}

void loginout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGIN_OUT_MSG;
    js["id"] = currentUser.getId();
    js["name"] = currentUser.getName();
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), buf.size(), 0);
    error_if(-1 == len, "send loginmsg msg error!!");
    if (-1 != len)
    {
        isMainMenuRunning = false;
    }
}

string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return string(date);
}

void error_if(bool condition, const char *errmsg)
{
    if (condition)
    {
        cerr << errmsg << endl;
    }
}