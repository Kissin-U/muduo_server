#include "chatserver.hpp"
#include "json.hpp"
#include <functional>
#include "chatservice.hpp"
#include <mavlink.h>

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

    // 注册信息回调 - 现在支持MAVLink和JSON两种协议
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量 - 支持高并发无人机连接
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
    if(conn->connected())
    {
        // 新连接建立，为此连接创建一个MavlinkCodec实例
        auto codec = std::make_shared<MavlinkCodec>();
        codec->setMavlinkMessageCallback(
            std::bind(&ChatServer::onMavlinkMessage, this, std::placeholders::_1, std::placeholders::_2)
        );
        
        // 将codec与连接关联
        string connName = conn->name();
        mavlinkCodecs_[connName] = codec;
        
        LOG_INFO << "New UAV connection established: " << connName;
    }
    else
    {
        // 客户端断开连接
        string connName = conn->name();
        
        // 移除对应的MavlinkCodec实例
        mavlinkCodecs_.erase(connName);
        
        // 客户端异常关闭，不发送json了，直接就关了
        ChatService::instance()->clientCloseException(conn);
        // 如果连接失败，就释放socket fd资源
        conn->shutdown();
        
        LOG_INFO << "UAV connection closed: " << connName;
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    // 智能协议识别：检查数据是MAVLink还是JSON
    if (buffer->readableBytes() > 0)
    {
        // 检查第一个字节是否为MAVLink起始标志
        uint8_t firstByte = static_cast<uint8_t>(buffer->peek()[0]);
        
        if (firstByte == MAVLINK_STX_MAVLINK1 || firstByte == MAVLINK_STX)
        {
            // MAVLink协议数据处理
            string connName = conn->name();
            auto it = mavlinkCodecs_.find(connName);
            
            if (it != mavlinkCodecs_.end())
            {
                // 使用MavlinkCodec处理接收到的MAVLink数据
                it->second->onMessage(conn, buffer);
            }
            else
            {
                LOG_ERROR << "No MavlinkCodec found for connection: " << connName;
                buffer->retrieveAll();
            }
        }
        else
        {
            // JSON协议数据处理 - 兼容地面站控制指令
            string buf = buffer->retrieveAllAsString();
            try 
            {
                // 数据的反序列化
                json js = json::parse(buf); // 解析JSON数据
                
                // 达到的目的：完全解耦网络模块的代码和业务模块的代码 利用回调的思想
                // 通过js["msgid"] 获取=》业务handler=》conn js time
                auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>()); 
                // 回调消息绑定好的事件处理器，来执行相应的业务处理
                // using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;
                msgHandler(conn, js, time);
            }
            catch (const json::exception& e)
            {
                LOG_ERROR << "JSON parse error: " << e.what() << ", data: " << buf;
            }
        }
    }
}

// MAVLink消息处理回调
void ChatServer::onMavlinkMessage(const TcpConnectionPtr &conn, const mavlink_message_t &msg)
{
    // 根据消息类型进行处理
    switch (msg.msgid)
    {
        case MAVLINK_MSG_ID_HEARTBEAT:
        {
            mavlink_heartbeat_t heartbeat;
            mavlink_msg_heartbeat_decode(&msg, &heartbeat);
            
            LOG_INFO << "Received HEARTBEAT from UAV sysid=" << static_cast<int>(msg.sysid)
                     << ", type=" << static_cast<int>(heartbeat.type)
                     << ", autopilot=" << static_cast<int>(heartbeat.autopilot)
                     << ", base_mode=" << static_cast<int>(heartbeat.base_mode)
                     << ", system_status=" << static_cast<int>(heartbeat.system_status);
            break;
        }
        case MAVLINK_MSG_ID_GPS_RAW_INT:
        {
            mavlink_gps_raw_int_t gps;
            mavlink_msg_gps_raw_int_decode(&msg, &gps);
            
            LOG_INFO << "Received GPS from UAV sysid=" << static_cast<int>(msg.sysid)
                     << ", lat=" << gps.lat / 1e7
                     << ", lon=" << gps.lon / 1e7
                     << ", alt=" << gps.alt / 1000.0
                     << ", fix_type=" << static_cast<int>(gps.fix_type);
            break;
        }
        case MAVLINK_MSG_ID_ATTITUDE:
        {
            mavlink_attitude_t attitude;
            mavlink_msg_attitude_decode(&msg, &attitude);
            
            LOG_INFO << "Received ATTITUDE from UAV sysid=" << static_cast<int>(msg.sysid)
                     << ", roll=" << attitude.roll
                     << ", pitch=" << attitude.pitch
                     << ", yaw=" << attitude.yaw;
            break;
        }
        default:
            LOG_DEBUG << "Received MAVLink message msgid=" << msg.msgid 
                      << " from UAV sysid=" << static_cast<int>(msg.sysid);
            break;
    }
    
    // TODO: 这里将来会调用UAVService来处理业务逻辑
    // ChatService::instance()->handleMavlinkMessage(conn, msg);
}
