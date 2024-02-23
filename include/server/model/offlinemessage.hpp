#ifndef OFFLINEMESSAGE_H
#define OFFLINEMESSAGE_H

#include <iostream>
#include <string>
using namespace std;

class OfflineMessage
{
public:
    bool insert(int userId, string message);
    bool remove(int userId);
    vector<string> query(int userId);
};

#endif // OFFLINEMESSAGE_H