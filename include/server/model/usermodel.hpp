#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

// User表的数据操作类
class UserModel
{
public:
    // User表的增加方法
    bool insert(User &user); // 引用传递
    // Model给业务层提供的都是对象

    // 根据用户id查询用户信息
    User query(int id); // 返回一个对象 
    // User user = _userModel.query(id);因为左面需要一个对象！ 所以你右面肯定得声明成一个返回对象的
    // 所以咱们就是为了根据用户id查到用户信息所对应的对象

    // 更新用户的状态信息
    bool updateState(User user); // 传进来user对象，返回bool值

    // 重置用户状态信息
    void resetState();
};

#endif // USERMODEL_H