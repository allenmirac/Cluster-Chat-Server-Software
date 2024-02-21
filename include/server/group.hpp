#ifndef GROUP_H
#define GROUP_H

#include <string>
#include <vector>
#include "groupuser.hpp"
using namespace std;

class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")
        : id_(id), name_(name), desc_(desc)
    {
    }

    int getId() const { return id_; }
    string getName() const { return name_; }
    string getDesc() const { return desc_; }
    vector<GroupUser> getGroupUsers() const { return users_; }

    void setId(int id) { id_ = id; }
    void setName(string name) { name_ = name; }
    void setDesc(string desc) { desc_ = desc; }
    void setUsers(vector<GroupUser> users) { users_ = users; }

private:
    int id_;
    string name_;
    string desc_;
    vector<GroupUser> users_;
};

#endif // GROUP_H