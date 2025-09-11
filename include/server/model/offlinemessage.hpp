#ifndef OFFLINEMESSAGE_H
#define OFFLINEMESSAGE_H

#include <iostream>
#include <string>
#include <queue>
#include <mutex>
using namespace std;

class OfflineMessage
{
public:
    bool insert(int userId, const string& message);
    bool remove(int userId);
    vector<string> query(int userId);
    std::mutex queueMutex_;
    queue<std::pair<int, std::string>> failedQueue_;
};

#endif // OFFLINEMESSAGE_H