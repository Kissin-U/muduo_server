#include "usermodel.hpp"
#include "db.h"
#include <iostream>

#include "json.hpp"
using json = nlohmann::json;

using namespace std;

// 插入数据库，记住插入的是对象，返回的是bool类型
bool UserModel::insert(User &user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取一下插入成功的用户数据生成的主键id
            user.setID(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

User UserModel::query(int id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // mysql_query() 仅对 SELECT，SHOW，EXPLAIN 或 DESCRIBE 语句返回一个资源标识符，如果查询执行不正确则返回 FALSE。
        // 要是查询不成功是返回flase的
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res); // mysql_fetch_row把这个资源的行拿出来，把res指向这个地址的一行都取出来
            if (row != nullptr)
            {
                User user;
                user.setID(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                // res是返回指针，内部肯定是动态开辟内存来申请资源实现的，因此要用完了释放资源
                mysql_free_result(res);
                return user;
            }
        }
    }
    return User();
    // 就是返回User默认构造哪些参数的User
}

bool UserModel::updateState(User user)
{
    // 1.组装sql语句
    char sql[1024] = {0}; // char sql[1024] = {0};: 这是声明一个字符数组 sql，用于存储 SQL 查询字符串。
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());
    // sprintf(sql, ...): 这个函数将格式化的字符串写入 sql 数组中。构建的 SQL 语句将会是类似于：update user set state = 'new_state' where id = user_id;

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql)) // mysql.update(sql) 这行代码才是实际执行 SQL 更新语句的部分
            return true;
    }
    return false;
}

// 重置用户状态信息
void UserModel::resetState()
{
    // 1.组装sql语句
    char sql[1024] = "update user set state = 'offline' where state = 'online'";
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql); // mysql.update(sql) 这行代码才是实际执行 SQL 更新语句的部分
    }
}