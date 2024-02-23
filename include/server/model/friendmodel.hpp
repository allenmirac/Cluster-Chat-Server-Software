#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include <vector>
#include <string>
#include "user.hpp"
using namespace std;

class FriendModel
{
public:
    void insert(int userid, int frindid);
    vector<User> query(int userid);
};

#endif // FRIENDMODEL_H