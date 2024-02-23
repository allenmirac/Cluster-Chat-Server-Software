#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

class User
{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline");
    void setId(int id);
    void setName(string name);
    void setPassword(string password);
    void setState(string state = "offline");

    int getId() const;
    string getName() const;
    string getPassword() const;
    string getState() const;

private:
    int id_;
    string name_;
    string password_;
    string state_;
};

#endif // USER_H