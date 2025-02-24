#include "groupmodel.hpp"
#include "db.h"

// 创建群组
bool GroupModel::createGroup(Group &group) // 向AllGroup表中增加信息
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')", // 他按照顺序插入，就不填id了，你不需要定义这个id
            group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取一下插入成功的用户数据生成的主键id
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}
// 加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values(%d, %d, '%s')",
            groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid) // 根据指定的groupid查询群组用户id列表，除了userid自己，主要用户群聊业务给群组其他成员群发消息
{
    /*
    1. 先根据userid再groupuser表中查询出该用户所属的群组信息
    2.再根据群组信息，查询属于该群组的所有用户的userid。并且和user表进行多表联合查询，查出用户的详细信息
    */

    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from allgroup a inner join groupuser b on a.id = b.groupid where b.userid = %d", userid);

    vector<Group> groupVec; // 这是一个装载Group对象的容器

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // mysql_query() 仅对 SELECT，SHOW，EXPLAIN 或 DESCRIBE 语句返回一个资源标识符，如果查询执行不正确则返回 FALSE。
        // 要是查询不成功是返回flase的
        if (res != nullptr)
        {
            MYSQL_ROW row;
            // 查出userid所有的群组信息，并且塞入容器中，返回去
            while ((row = mysql_fetch_row(res)) != nullptr) // mysql_fetch_row(res)返回一个 MYSQL_ROW 指针，指向结果集中的下一行数据
            {
                Group group;
                group.setId(atoi(row[0])); // atoi 将 C 风格字符串转换为整数（int）,row[0]第4列
                group.setName((row[1]));   // 第2列
                group.setDesc((row[2]));
                groupVec.push_back(group); // user压入vec容器中 push_back 会在容器末尾添加一个新元素。
            }
            mysql_free_result(res);
            return groupVec;
        }
    }
    return groupVec;

    // 查询群组的用户信息
    for (Group &group : groupVec) // groupVec就是个group类的数组，明白了吧
    // groupvec中全是group对象，Group &group是groupvec中group对象的引用
    {
        // 现在是把当前group的所有user都查出来，是不是就得是groupuser与user联合查询？
        sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a inner join groupuser b on b.userid = a.id where b.groupid = %d", group.getId());

        MySQL mysql;
        if (mysql.connect())
        {
            MYSQL_RES *res = mysql.query(sql); // mysql_query() 仅对 SELECT，SHOW，EXPLAIN 或 DESCRIBE 语句返回一个资源标识符，如果查询执行不正确则返回 FALSE。
            // 要是查询不成功是返回flase的
            if (res != nullptr)
            {
                MYSQL_ROW row;
                // 查出userid所有的群组信息，并且塞入容器中，返回去
                while ((row = mysql_fetch_row(res)) != nullptr) // mysql_fetch_row(res)返回一个 MYSQL_ROW 指针，指向结果集中的下一行数据
                {
                    GroupUser user;
                    user.setID(atoi(row[0])); // atoi 将 C 风格字符串转换为整数（int）,row[0]第4列
                    user.setName((row[1]));   // 第2列
                    user.setState((row[2]));
                    user.setRole((row[3]));
                    group.getUsers().push_back(user); // user压入group的getusers中，就是个vec容器中 push_back 会在容器末尾添加一个新元素。
                }
                mysql_free_result(res);
            }
        }
    }
    return groupVec;
}

// 根据指定的groupid查询群组用户id列表，除了userid自己，主要用户群聊业务给群组其他成员群发消息
vector<int> GroupModel::queryGroupsUsers(int userid, int groupid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);

    vector<int> idVec; // 这是一个装载int id的容器

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // mysql_query() 仅对 SELECT，SHOW，EXPLAIN 或 DESCRIBE 语句返回一个资源标识符，如果查询执行不正确则返回 FALSE。
        // 要是查询不成功是返回flase的
        if (res != nullptr)
        {
            MYSQL_ROW row;
            // 查出userid所有的群组信息，并且塞入容器中，返回去
            while ((row = mysql_fetch_row(res)) != nullptr) // mysql_fetch_row(res)返回一个 MYSQL_ROW 指针，指向结果集中的下一行数据
            {
                idVec.push_back(atoi(row[0])); // 查到的id从string转换成int压入到idVec，咱们这一步只需要获取到除了userid以外的群员id
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}