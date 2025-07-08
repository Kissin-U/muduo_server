#include "mavlink_codec.hpp"
#include <muduo/base/Logging.h>
#include <cstring>

using json = nlohmann::json; // JSON库别名

MavlinkCodec::MavlinkCodec()
    : system_id_(255),      // 地面站通常使用255作为System ID
      component_id_(0)      // Component ID设为0
{
    // 初始化MAVLink解析状态
    memset(&mavlink_status_, 0, sizeof(mavlink_status_));
}

MavlinkCodec::~MavlinkCodec()
{
}

void MavlinkCodec::onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf)
{
    // 从缓冲区中逐字节读取数据并解析MAVLink消息
    while (buf->readableBytes() > 0)
    {
        uint8_t byte = static_cast<uint8_t>(buf->readInt8());
        mavlink_message_t msg;
        
        // 使用MAVLink库解析字节
        uint8_t msg_received = mavlink_parse_char(MAVLINK_COMM_0, byte, &msg, &mavlink_status_);
        
        if (msg_received)
        {
            // 成功解析出一个完整的MAVLink消息
            LOG_INFO << "Received MAVLink message: msgid=" << msg.msgid 
                     << ", sysid=" << static_cast<int>(msg.sysid)
                     << ", compid=" << static_cast<int>(msg.compid)
                     << ", seq=" << static_cast<int>(msg.seq);
            
            // 调用回调函数处理消息
            if (mavlinkMessageCallback_)
            {
                mavlinkMessageCallback_(conn, msg);
            }
        }
    }
}

void MavlinkCodec::sendMavlinkMessage(const muduo::net::TcpConnectionPtr& conn, const mavlink_message_t& msg)
{
    // 将MAVLink消息序列化为字节流
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    
    if (len > 0)
    {
        // 发送字节流到TCP连接
        conn->send(buffer, len);
        
        LOG_INFO << "Sent MAVLink message: msgid=" << msg.msgid 
                 << ", sysid=" << static_cast<int>(msg.sysid)
                 << ", compid=" << static_cast<int>(msg.compid)
                 << ", len=" << len;
    }
    else
    {
        LOG_ERROR << "Failed to serialize MAVLink message: msgid=" << msg.msgid;
    }
}

// MAVLink消息转换为JSON格式 - 用于Redis发布/订阅和Web前端
json MavlinkCodec::mavlinkToJson(const mavlink_message_t& msg)
{
    json js;
    
    // 通用字段
    js["msgid"] = msg.msgid;
    js["sysid"] = msg.sysid;
    js["compid"] = msg.compid;
    js["seq"] = msg.seq;
    js["timestamp"] = std::time(nullptr); // 添加时间戳
    
    // 根据消息类型解析具体数据
    switch (msg.msgid)
    {
        case MAVLINK_MSG_ID_HEARTBEAT:
        {
            mavlink_heartbeat_t heartbeat;
            mavlink_msg_heartbeat_decode(&msg, &heartbeat);
            
            js["type"] = "heartbeat";
            js["data"]["vehicle_type"] = heartbeat.type;
            js["data"]["autopilot"] = heartbeat.autopilot;
            js["data"]["base_mode"] = heartbeat.base_mode;
            js["data"]["custom_mode"] = heartbeat.custom_mode;
            js["data"]["system_status"] = heartbeat.system_status;
            js["data"]["mavlink_version"] = heartbeat.mavlink_version;
            break;
        }
        case MAVLINK_MSG_ID_GPS_RAW_INT:
        {
            mavlink_gps_raw_int_t gps;
            mavlink_msg_gps_raw_int_decode(&msg, &gps);
            
            js["type"] = "gps";
            js["data"]["time_usec"] = gps.time_usec;
            js["data"]["fix_type"] = gps.fix_type;
            js["data"]["lat"] = gps.lat / 1e7;  // 转换为度
            js["data"]["lon"] = gps.lon / 1e7;  // 转换为度
            js["data"]["alt"] = gps.alt / 1000.0; // 转换为米
            js["data"]["eph"] = gps.eph;
            js["data"]["epv"] = gps.epv;
            js["data"]["vel"] = gps.vel;
            js["data"]["cog"] = gps.cog;
            js["data"]["satellites_visible"] = gps.satellites_visible;
            break;
        }
        case MAVLINK_MSG_ID_ATTITUDE:
        {
            mavlink_attitude_t attitude;
            mavlink_msg_attitude_decode(&msg, &attitude);
            
            js["type"] = "attitude";
            js["data"]["time_boot_ms"] = attitude.time_boot_ms;
            js["data"]["roll"] = attitude.roll;
            js["data"]["pitch"] = attitude.pitch;
            js["data"]["yaw"] = attitude.yaw;
            js["data"]["rollspeed"] = attitude.rollspeed;
            js["data"]["pitchspeed"] = attitude.pitchspeed;
            js["data"]["yawspeed"] = attitude.yawspeed;
            break;
        }
        default:
            js["type"] = "unknown";
            js["data"]["raw_msgid"] = msg.msgid;
            break;
    }
    
    return js;
}

// JSON指令转换为MAVLink消息 - 用于地面站控制指令
mavlink_message_t MavlinkCodec::jsonToMavlink(const json& js)
{
    mavlink_message_t msg;
    memset(&msg, 0, sizeof(msg));
    
    try 
    {
        std::string command_type = js["command"].get<std::string>();
        uint8_t target_system = js.value("target_system", 1);
        uint8_t target_component = js.value("target_component", 1);
        
        if (command_type == "arm")
        {
            // 解锁指令
            mavlink_msg_command_long_pack(
                255, 0, &msg,           // 地面站ID
                target_system, target_component,
                MAV_CMD_COMPONENT_ARM_DISARM,
                0,                      // confirmation
                1,                      // param1: 1=arm, 0=disarm
                0, 0, 0, 0, 0, 0       // param2-7
            );
        }
        else if (command_type == "disarm")
        {
            // 上锁指令
            mavlink_msg_command_long_pack(
                255, 0, &msg,
                target_system, target_component,
                MAV_CMD_COMPONENT_ARM_DISARM,
                0,
                0,                      // param1: 0=disarm
                0, 0, 0, 0, 0, 0
            );
        }
        else if (command_type == "takeoff")
        {
            // 起飞指令
            float altitude = js.value("altitude", 10.0f);
            mavlink_msg_command_long_pack(
                255, 0, &msg,
                target_system, target_component,
                MAV_CMD_NAV_TAKEOFF,
                0,
                0,                      // param1: pitch
                0,                      // param2: empty
                0,                      // param3: empty
                0,                      // param4: yaw
                0, 0,                   // param5-6: lat, lon (0=current)
                altitude                // param7: altitude
            );
        }
        else if (command_type == "rtl")
        {
            // 返航指令
            mavlink_msg_command_long_pack(
                255, 0, &msg,
                target_system, target_component,
                MAV_CMD_NAV_RETURN_TO_LAUNCH,
                0,
                0, 0, 0, 0, 0, 0, 0    // 所有参数为0
            );
        }
        else
        {
            LOG_ERROR << "Unknown command type: " << command_type;
        }
    }
    catch (const json::exception& e)
    {
        LOG_ERROR << "JSON to MAVLink conversion error: " << e.what();
    }
    
    return msg;
}
