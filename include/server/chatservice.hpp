#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "redis.hpp"
#include "groupmodel.hpp"
#include "friendmodel.hpp"
#include "usermodel.hpp"
#include "offlinemessgaemodel.hpp"
#include "json.hpp"
using json = nlohmann::json;

// 就相当于Define定义新的类型名称 给了一个包装器，创建了一个名为 MsgHandler 的类型别名。
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>; // 时间id对应的回调
// 封装后，您可以像调用普通函数一样调用 MsgHandler，传入相应的参数。

// 聊天服务器业务类 单例模式

// 因为网络模块和业务模块解耦，所以要在这里给msgid映射一个事件回调，就是当网络模块读出来msgid后就传给业务模块来操作
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService *instance(); // instance函数的声明
    // 返回一个指向 ChatService 类的指针
    // 处理登陆业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time); // const TcpConnectionPtr &conn 当您希望确保传入的对象在函数内不会被修改时，使用常量引用
    // const TcpConnectionPtr conn 是一个常量指针的副本，您不能修改指针本身的值，但可以修改它所指向的对象

    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 服务器异常退出ctrl+c这种，业务重置方法
    void reset();

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 获取消息对应的处理器
    // 声明一个getHandler函数，是MsgHandler类型的
    MsgHandler getHandler(int msgid); // getHandler 函数返回的是一个 MsgHandler 类型的函数对象。具体来说，这个 MsgHandler 是一个函数类型的别名

    // 获取订阅消息业务
    void handleRedisSubscribeMessage(int userid, string msg);

    

private:
    // 这个类没有在public中声明构造函数，而是在私有函数中声明了构造函数
    // 私有构造函数，防止外部实例化
    ChatService(); // 构造函数的声明

    // 存储事件id和其对应业务的处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap; // 消息处理器的一个表，写消息id对应的处理操作

    // 存储在线用户的通信连接 这个是一个单例模式，多线程操作这个通信连接的时候，肯得得考虑线程安全！
    unordered_map<int, TcpConnectionPtr> _userConnMap; // 连接会随着进程而改变，要考虑进程安全问题
    // 比如现在存的指针，还没执行完毕呢，就被第二个线程更改了，这是不行的，因此得上一个互斥锁

    // 定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;

    // 数据操作类对象s
    UserModel _userModel;

    // 离线消息操作对象
    OfflineMsgModel _offlineMsgModel;

    // 好友信息操作对象
    FriendModel _friendModel;

    // 群组信息操作对象
    GroupModel _groupModel;
    
    // redis操作对象
    Redis _redis;
};

#endif