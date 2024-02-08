#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

class UserModel
{
public:
    bool insert(User &user);
    User query(int id);
    void updateState(User &user);
};

#endif // USERMODEL_H