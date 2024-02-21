#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <vector>

class GroupModel
{
public:
    void createGroup(Group &group);
    void joinGroup(int userid, int groupid, string role="normal");
    vector<Group> queryGroups(int userid);
    vector<int> queryGroupUsers(int userid, int groupid);
};
#endif // GROUPMODEL_H