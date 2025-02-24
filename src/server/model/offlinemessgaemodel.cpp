#include "offlinemessgaemodel.hpp"
#include "db.h" // 包含数据库

// 存储用户离线消息
void OfflineMsgModel::insert(int userid, string msg)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str()); // 没有指定列名，就按照顺序插入了，插第一列和第二列

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
// 删除用户离线消息
void OfflineMsgModel::remove(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d", userid); // 没有指定列名，就按照顺序插入了，插第一列和第二列

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户离线消息
vector<string> OfflineMsgModel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);

    vector<string> vec;
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
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
    // 就是返回User默认构造哪些参数的User
}
