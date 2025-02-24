#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

class GroupUser : public User // groupuser继承user user是基类，groupuser是子类，因为groupuser包含user的信息，但是还需要假如grouprole这个角色
// groupuser的前提他就是个user，只不过是个细化的user
{
public:
    void setRole(string role) { this->role = role; }
    string getrole() { return this->role; }

private:
    // grouprole只是添加了一个派生类的特殊成员变量role
    string role;
};

#endif