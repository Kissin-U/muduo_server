#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <mavlink.h>

int main()
{
    // 创建socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(6000);  // 假设服务器监听6000端口
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 连接到服务器
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        close(sock);
        return -1;
    }

    std::cout << "Connected to UAV server" << std::endl;

    // 创建并发送心跳包
    mavlink_message_t msg;
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

    // 模拟PX4无人机的心跳包
    mavlink_msg_heartbeat_pack(
        1,                          // system_id (无人机ID)
        1,                          // component_id (组件ID)
        &msg,                       // 消息结构体
        MAV_TYPE_QUADROTOR,         // 无人机类型：四旋翼
        MAV_AUTOPILOT_PX4,          // 自驾仪类型：PX4
        MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED | MAV_MODE_FLAG_SAFETY_ARMED,  // 基础模式
        0,                          // 自定义模式
        MAV_STATE_ACTIVE            // 系统状态：激活
    );

    // 序列化消息
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);

    // 发送心跳包
    for (int i = 0; i < 5; i++) {
        ssize_t bytes_sent = send(sock, buffer, len, 0);
        if (bytes_sent < 0) {
            std::cerr << "Send failed" << std::endl;
            break;
        }
        
        std::cout << "Sent heartbeat #" << (i+1) << " (" << bytes_sent << " bytes)" << std::endl;
        sleep(1);  // 每秒发送一次心跳
    }

    // 发送GPS数据
    mavlink_msg_gps_raw_int_pack(
        1,                          // system_id
        1,                          // component_id
        &msg,                       // 消息结构体
        0,                          // time_usec (微秒时间戳)
        3,                          // fix_type (3D fix)
        39.9042 * 1e7,             // lat (纬度，北京天安门)
        116.4074 * 1e7,            // lon (经度，北京天安门)
        50000,                      // alt (海拔，毫米)
        65535,                      // eph (水平精度)
        65535,                      // epv (垂直精度)
        65535,                      // vel (速度)
        65535,                      // cog (航向)
        10                          // satellites_visible (可见卫星数)
    );

    len = mavlink_msg_to_send_buffer(buffer, &msg);
    send(sock, buffer, len, 0);
    std::cout << "Sent GPS data" << std::endl;

    // 发送姿态数据
    mavlink_msg_attitude_pack(
        1,                          // system_id
        1,                          // component_id
        &msg,                       // 消息结构体
        0,                          // time_boot_ms (启动时间)
        0.1,                        // roll (横滚角)
        0.05,                       // pitch (俯仰角)
        1.57,                       // yaw (偏航角)
        0.01,                       // rollspeed (横滚角速度)
        0.01,                       // pitchspeed (俯仰角速度)
        0.01                        // yawspeed (偏航角速度)
    );

    len = mavlink_msg_to_send_buffer(buffer, &msg);
    send(sock, buffer, len, 0);
    std::cout << "Sent attitude data" << std::endl;

    std::cout << "Test completed. Closing connection." << std::endl;
    close(sock);
    return 0;
}
