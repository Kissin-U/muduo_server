#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include <string>
#include <vector>
using namespace std;

//User表的ORM类
class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")    
    {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }

    void setId(int id){this->id = id;}
    void setName(string name){this->name = name;}
    void setDesc(string desc){this->desc = desc;}

    int getId(){return this->id;}
    string getName(){return this->name;}
    string getDesc(){return this->desc;}
    vector<GroupUser> &getUsers() {return this->users;} // 命名叫做getUsers返回的是vec容器的指针里面装的是GroupUsers类的vec容器，每个groupuser里面都有user+role
    // 里面都是值string啦很多东西，得返回引用避免开销返回值还得拷贝，资源占用太大了

private:
    int id;
    string name;
    string desc;
    vector<GroupUser> users; // 查出来的组的成员都放在这个vector中，给业务层去使用
};

#endif