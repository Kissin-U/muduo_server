# MAVLink集成使用指南

## 概述

本项目已成功集成MAVLink协议，支持与PX4、ArduPilot等主流飞控系统通信。服务器能够智能识别MAVLink和JSON两种协议，实现无人机数据采集、协议转换和Web前端展示。

## 数据流架构

### 完整数据流向
```
无人机 --MAVLink--> 服务器 --JSON--> Web前端显示
                     ↓
                  数据库存储
                     ↓
                  Redis同步
```

```
Web前端 --JSON--> 服务器 --MAVLink--> 无人机执行
```

## 主要功能

### 1. 协议智能识别
- **MAVLink v2.0**: 完全兼容MAVLink协议标准，用于无人机通信
- **JSON**: 用于Web前端交互和数据展示
- **智能识别**: 通过数据包首字节自动识别协议类型，无需手动配置

### 2. 支持的MAVLink消息类型
- `HEARTBEAT`: 无人机心跳包，包含飞行器类型、自驾仪类型、飞行模式等
- `GPS_RAW_INT`: GPS原始数据，包含位置、高度、精度等信息
- `ATTITUDE`: 姿态数据，包含横滚、俯仰、偏航角度和角速度

### 3. 协议转换功能
- **MAVLink → JSON**: 将无人机遥测数据转换为JSON格式，用于：
  - Web前端实时显示
  - 数据库存储
  - Redis发布/订阅同步
- **JSON → MAVLink**: 将Web前端控制指令转换为MAVLink命令发送给无人机

## 使用方法

### 1. 启动服务器
```bash
cd muduo_server/bin
./ChatServer
```

### 2. 无人机连接
无人机通过TCP连接到服务器（默认端口6000），发送标准MAVLink数据包：

```cpp
// 示例：发送心跳包
mavlink_message_t msg;
mavlink_msg_heartbeat_pack(
    1,                          // system_id
    1,                          // component_id  
    &msg,
    MAV_TYPE_QUADROTOR,         // 四旋翼
    MAV_AUTOPILOT_PX4,          // PX4飞控
    base_mode,                  // 飞行模式
    custom_mode,                // 自定义模式
    MAV_STATE_ACTIVE            // 系统状态
);
```

### 3. 地面站控制
地面站可以发送JSON格式的控制指令：

```json
{
    "command": "arm",
    "target_system": 1,
    "target_component": 1
}
```

支持的控制指令：
- `arm`: 解锁无人机
- `disarm`: 上锁无人机  
- `takeoff`: 起飞（需要指定altitude参数）
- `rtl`: 返航

### 4. 数据格式示例

#### MAVLink转JSON示例
```json
{
    "msgid": 0,
    "sysid": 1,
    "compid": 1,
    "seq": 123,
    "timestamp": 1641234567,
    "type": "heartbeat",
    "data": {
        "vehicle_type": 2,
        "autopilot": 12,
        "base_mode": 81,
        "custom_mode": 0,
        "system_status": 4,
        "mavlink_version": 3
    }
}
```

#### GPS数据JSON格式
```json
{
    "msgid": 24,
    "type": "gps",
    "data": {
        "lat": 39.9042,
        "lon": 116.4074,
        "alt": 50.0,
        "fix_type": 3,
        "satellites_visible": 10
    }
}
```

## 技术架构

### 1. 核心组件
- **MavlinkCodec**: MAVLink协议编解码器
- **ChatServer**: 网络服务器，支持双协议
- **协议识别**: 基于数据包头部自动识别协议类型

### 2. 设计特点
- **高性能**: 基于muduo网络库，支持高并发连接
- **模块化**: 协议处理与业务逻辑分离
- **可扩展**: 易于添加新的MAVLink消息类型支持
- **向下兼容**: 保留原有JSON协议功能

### 3. 文件结构
```
muduo_server/
├── include/server/
│   ├── mavlink_codec.hpp      # MAVLink编解码器头文件
│   └── chatserver.hpp         # 服务器头文件
├── src/server/
│   ├── mavlink_codec.cpp      # MAVLink编解码器实现
│   └── chatserver.cpp         # 服务器实现
├── thirdparty/mavlink/        # MAVLink库
└── test/
    └── mavlink_test_client.cpp # 测试客户端
```

## 测试

### 1. 编译测试客户端
```bash
cd muduo_server/build
make MavlinkTestClient
```

### 2. 运行测试
```bash
# 启动服务器
./bin/ChatServer

# 运行测试客户端（另一个终端）
./bin/MavlinkTestClient
```

测试客户端会模拟PX4无人机发送心跳包、GPS数据和姿态数据。

## 日志输出

服务器会详细记录接收到的MAVLink消息：

```
INFO: Received HEARTBEAT from UAV sysid=1, type=2, autopilot=12, base_mode=81, system_status=4
INFO: Received GPS from UAV sysid=1, lat=39.9042, lon=116.4074, alt=50, fix_type=3
INFO: Received ATTITUDE from UAV sysid=1, roll=0.1, pitch=0.05, yaw=1.57
```

## 后续扩展

1. **更多消息类型**: 添加对更多MAVLink消息的支持
2. **UAVService**: 实现专门的无人机业务逻辑层
3. **编队管理**: 支持多无人机编队控制
4. **Redis集成**: 实现跨服务器的无人机数据同步
5. **Web界面**: 开发实时监控界面

## 注意事项

1. 确保无人机配置正确的地面站IP和端口
2. 防火墙需要开放相应端口
3. 建议在局域网环境下测试
4. 生产环境需要考虑安全认证机制
