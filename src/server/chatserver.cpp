#include "chatserver.hpp"
#include "json.hpp"
#include <functional>
#include "chatservice.hpp"

#include <string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json; // 用了别名 

ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg) // ChatServer::ChatServer 表示这是 ChatServer 类的构造函数 您在类外实现构造函数，您需要使用 :: 来指明这个函数属于哪个类。例如：
    : _server(loop, listenAddr, nameArg), _loop(loop) // 类外构造函数+初始化列表
    //构造常量（const）或引用类型的成员: 常量和引用必须在初始化列表中进行初始化，因为它们在构造函数体内无法被赋值。
{
    // 注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册信息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}
// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 使用智能指针作为参数时，通常选择通过引用传递
// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn) // const TcpConnectionPtr &conn 表示一个指向 TcpConnection 对象的常量引用
{
    // 客户端断开连接
    if(!conn->connected())
    {
        // 客户端异常关闭，不发送json了，直接就关了
        ChatService::instance()->clientCloseException(conn);
        // 如果连接失败，就释放socket fd资源
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString(); 
    // 数据的反序列化
    json js = json::parse(buf); // 要不就是nlohmann::json：：parse(buf)
    // 达到的目的：完全解耦网络模块的代码和业务模块的代码 利用回调的思想
    // 通过js["msgid"] 获取=》业务handler=》conn js time
    auto msgHandler = ChatService::instance() -> getHandler(js["msgid"].get<int>()); // 现在是得到一个js对象而不是int,调用json的get方法，用int实例化强制转换成int类型
    // 回调消息绑定好的事件处理器，来执行相应的业务处理
    // using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;
    msgHandler(conn, js, time);
}