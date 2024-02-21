#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

class GroupUser : public User
{
public:
    string getRole() { return role_; }
    void setRole(string role) { role_ = role; }

private:
    string role_;
};

#endif // GROUPUSER_H