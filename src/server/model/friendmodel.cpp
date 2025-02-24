#include "friendmodel.hpp"
#include "db.h"

// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 返回用户好友列表 friendid做两个表的联合查询
vector<User> FriendModel::query(int userid)
{
        // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d", userid);

    vector<User> vec; // 这是一个装载User对象的容器
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // mysql_query() 仅对 SELECT，SHOW，EXPLAIN 或 DESCRIBE 语句返回一个资源标识符，如果查询执行不正确则返回 FALSE。
        // 要是查询不成功是返回flase的
        if (res != nullptr)
        {
            // 把userid用户中所有离线消息都放入vector容器中，并且返回Vector
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) // mysql_fetch_row(res)返回一个 MYSQL_ROW 指针，指向结果集中的下一行数据
            {
                User user;
                user.setID(atoi(row[0])); // atoi 将 C 风格字符串转换为整数（int）,row[0]第4列
                user.setName((row[1])); // 第2列
                user.setState((row[2]));
                vec.push_back(user); // user压入vec容器中 push_back 会在容器末尾添加一个新元素。
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}