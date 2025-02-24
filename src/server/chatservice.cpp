#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <vector>

using namespace std;
using namespace muduo;

// 首先实现这个方法 这个方法是静态的，直接不需要创建类了，直接加上域就行了
ChatService *ChatService::instance()
// 函数语法是 函数返回类型 函数名，而现在instance所属域是Chatservice
// 因此，首先我们确定instance返回类型是一个ChatService指针，第二所属是Chatservice
// 因此就ChatService* ChatService::instance()，这不是函数重载，是域外实现函数，重载是指在同一个作用域中定义多个同名但参数不同的函数。
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的handler回调操作
ChatService::ChatService() // 类外构造函数
{
    // 如果不绑定，不能直接调用函数指针，需要一个对象实例作为上下文
    // 为了正确调用成员函数指针，您需要将其绑定到一个特定的对象实例上。可以使用 std::bind 或 lambda 表达式来实现这一点。
    // // 使用 std::bind 将成员函数与当前对象绑定
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    // 其实就是调用的ChatService.login insert({LOGIN_MSG,ChatService.login）

    _msgHandlerMap.insert({LOGIN_OUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});

    // 也得有注册的
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    // 要回调就得注册
    // 注册过程将特定事件（如消息类型、用户操作等）与对应的处理函数（回调函数）关联起来
    // 这个就是将reg与当前ChatService类绑定，这样通过map中REG_MSG一一对应来调取

    // 注册一下ONE_CHAT_MSG的handle处理，也是跟ChatService的oneChat与当前类绑定一下
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});

    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});

    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});

    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 链接Redis服务器
    if (_redis.connect())
    {
        // 设置上报信息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把online状态的用户，设置成offline
    _userModel.resetState();
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
// 不能省略MsgHandler返回类型
// 返回类型 类名::函数名(参数列表)。
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器，空操作，lambda函数，自动隐式值捕获
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            // 用muduo库的打印，不用cout
            LOG_ERROR << "Msgid : " << msgid << " can not find handler!";
        };
    }
    else
        return _msgHandlerMap[msgid];
}

// 实现登录业务 id pwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);

    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登陆，不允许重复登陆
            json response;
            response["msgid"] = LOGIN_MSG_ACK;                         // 这一步说明是已经响应了
            response["errno"] = 2;                                     // 为1说明注册失败
            response["errmsg"] = "该账号已经登录，请重新输入新的账号"; //
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            } // 连接的用户上线，用户线下，线程安全问题，多线程访问

            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            // 登录成功，更新用户状态信息 State从offline变成online
            user.setState("online");
            _userModel.updateState(user);
            // mysql不用担心线程安全，他都是由mysql负责线程安全

            json response;
            response["msgid"] = LOGIN_MSG_ACK; // 这一步说明是已经响应了
            response["errno"] = 0;             // 为零说明成功了，为1说明有Erro
            response["id"] = user.getId();
            response["name"] = user.getName();
            response["state"] = user.getState();
            // 检查用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty()) // vec容器不为空，说明有离线消息，应该转发了
            {
                response["offlinemsg"] = vec; // vector可以直接给json
                // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id); // 咱们这里写的query返回的就是vec
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;                       // 定义一个json对象js
                    js["id"] = user.getId();       // Id添加到 JSON 对象中
                    js["name"] = user.getName();   // Name添加到 JSON 对象中
                    js["state"] = user.getState(); // State添加到 JSON 对象中
                    vec2.push_back(js.dump());     // JSON 对象转换成字符串
                }
                // response["friends"] = userVec; // 这个vec里面装的是对象，他不是string,是自定义类型！不能直接给json
                response["friends"] = vec2;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在，或者用户存在但是密码错误，登陆失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK; // 这一步说明是已经响应了
        response["errno"] = 1;             // 为1说明登录失败
        response["errmsg"] = "用户名或密码错误";

        conn->send(response.dump());
    }
}
// 实现注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user); // 插入数据库中
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK; // 这一步说明是已经响应了
        response["errno"] = 0;           // 为零说明成功了，为1说明有Erro
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK; // 这一步说明是已经响应了
        response["errno"] = 1;           // 为1说明注册失败
        response["id"] = user.getId();
        conn->send(response.dump());
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    User user(userid, "", "", "offline");
    user.setState("offline");
    // 把自己定义user的信息传入到正经的usermodel中
    _userModel.updateState(user); // 咱们这个user已经获取到id了，可以直接对象传给usermodel的updatestate中，他根据id再更新state}
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{

    // 咱们是先在map表中查找
    // 存储用户在线链接 unordered_map<int, TcpConnectionPtr> _userConnMap; 在map表中去查整个connection
    // 1.找到这个connection对应的int就是id，然后从map中删除，表示不存储在线链接了，要注意线程安全
    // 2.把这个id对应的state改成offline
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 设定了一个user对象，得用user对象的id，接一下查到这个connection的int
                // 从map表中删除用户的连接信息
                user.setID(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        // 把自己定义user的信息传入到正经的usermodel中
        _userModel.updateState(user); // 咱们这个user已经获取到id了，可以直接对象传给usermodel的updatestate中，他根据id再更新state}
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid;
    toid = js["toid"].get<int>();
    // 涉及到动状态信息了吧，得线程安全
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息,服务器主动推给toid用户
            it->second->send(js.dump()); // send是将信息传给buffer中了
            return;
        }
    }

    // 查toid在不在线，如果在线说明是没在这台服务器上得利用中间件redis的publish到toid的channel上，这样toid订阅的channel就接收到了
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }
    // toid不在线，存储离线消息，等他上线再给他
    _offlineMsgModel.insert(toid, js.dump()); // js.dump() 将 JSON 对象序列化为字符串
}

// 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 创建群组肯定得是json对象传进来，咱们创建群组逻辑，首先是谁是群主？谁创建的群组？因此需要一个userid，然后群名，群功能，需要groupname和groupdesc
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 定义完所需变量了，下一步就是存储群组信息，存储在哪里呢？存储在Group类中
    Group group(-1, name, desc);
    // 就是看是否已经创建了，如果创建了直接加就行了
    if (_groupModel.createGroup(group)) // creatGroup返回的是bool类型，如果创建成功了就返回true
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 创建群组聊天
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupsUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec) // 就是给除了自己以外其他人一个一个发
    {

        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 其他id在线，转发消息,服务器主动推给toid用户
            it->second->send(js.dump()); // send是将信息传给buffer中了
        }
        else
        {
            // 查询toid是否在线
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
                // toid不在线，存储离线消息，等他上线再给他
                _offlineMsgModel.insert(id, js.dump()); // js.dump() 将 JSON 对象序列化为字符串
        }
    }
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        // 其他id在线，转发消息,服务器主动推给toid用户
        it->second->send(msg); // send是将信息传给buffer中了
        return;
    }
    _offlineMsgModel.insert(userid, msg);
}
