#include <iostream>
#include <unordered_map>
#include <functional>
using namespace std;

bool isMainMenuRunning = true;
// 获取主菜单
void mainmenu(int);

int main()
{
    cout << "Hello world" << endl;
    mainmenu(0);
    return 0;
}

void help(int fd = 0, string str = "");

unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addFriend", "添加好友，格式addFriend:friendid"},
    {"createGroup", "创建群聊，格式createGroup:groupName:groupDesc"},
    {"joinGroup", "加入群聊，格式joinGroup:groupid"},
    {"groupChat", "群聊，格式groupChat:groupid:message"},
    {"loginout", "注销，格式loginout"}};

unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help}

};

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
