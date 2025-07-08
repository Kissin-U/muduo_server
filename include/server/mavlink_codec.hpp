#ifndef MAVLINK_CODEC_HPP
#define MAVLINK_CODEC_HPP

#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <functional>
#include <mavlink.h>
#include "json.hpp"

class MavlinkCodec
{
public:
    // MAVLink消息处理回调函数类型
    using MavlinkMessageCallback = std::function<void(const muduo::net::TcpConnectionPtr&, const mavlink_message_t&)>;

    MavlinkCodec();
    ~MavlinkCodec();

    // 设置MAVLink消息处理回调
    void setMavlinkMessageCallback(const MavlinkMessageCallback& cb)
    {
        mavlinkMessageCallback_ = cb;
    }

    // 处理从网络接收到的数据
    void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf);

    // 发送MAVLink消息到连接
    void sendMavlinkMessage(const muduo::net::TcpConnectionPtr& conn, const mavlink_message_t& msg);

    // MAVLink消息转换为JSON格式 - 用于Redis发布/订阅和Web前端
    static nlohmann::json mavlinkToJson(const mavlink_message_t& msg);
    
    // JSON指令转换为MAVLink消息 - 用于地面站控制指令
    static mavlink_message_t jsonToMavlink(const nlohmann::json& js);

private:
    MavlinkMessageCallback mavlinkMessageCallback_;
    mavlink_status_t mavlink_status_;  // MAVLink解析状态
    uint8_t system_id_;               // 本系统的System ID
    uint8_t component_id_;            // 本系统的Component ID
};

#endif // MAVLINK_CODEC_HPP
