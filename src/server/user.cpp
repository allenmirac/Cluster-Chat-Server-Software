#include "user.hpp"

User::User(int id, string name, string password, string state) : id_(id), name_(name), password_(password), state_(state)
{
}

void User::setId(int id)
{
    id_ = id;
}
void User::setName(string name)
{
    name_ = name;
}
void User::setPassword(string password)
{
    password_ = password;
}
void User::setState(string state)
{
    state_ = state;
}

int User::getId() const
{
    return id_;
}
string User::getName() const
{
    return name_;
}
string User::getPassword() const
{
    return password_;
}
string User::getState() const
{
    return state_;
}
