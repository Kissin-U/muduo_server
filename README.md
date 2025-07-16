# 基于Muduo网络库的无人机通信服务器

一个高性能的无人机通信服务器，支持MAVLink和JSON双协议智能识别与处理，专门用于无人机集群数据采集和监控。

## 项目特点

- **双协议智能识别**: 自动识别MAVLink和JSON协议，无需预配置
- **高性能网络**: 基于Muduo网络库，支持大量并发连接
- **协议双向转换**: MAVLink ↔ JSON无缝转换
- **分布式数据同步**: 集成Redis发布/订阅，支持集群部署
- **线程安全**: 每连接独立编解码器，完善的并发控制

## 核心功能

### 数据流向
- **上行数据**: 无人机(MAVLink) → 服务器 → 转换成JSON → Web前端显示
- **下行控制**: Web前端(JSON) → 服务器 → 转换成MAVLink → 无人机执行

### 协议转换用途
- **MAVLink → JSON**: 便于Web前端显示、数据库存储、Redis同步
- **JSON → MAVLink**: 将用户操作转换为无人机标准控制指令

## 技术栈

- **网络库**: Muduo (基于Reactor模式)
- **协议**: MAVLink v2.0 + JSON
- **数据同步**: Redis (发布/订阅)
- **数据存储**: MySQL
- **负载均衡**: Nginx TCP Stream
- **构建**: CMake
- **容器**: Docker + Docker Compose

## 快速开始

### 编译
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 运行
```bash
# 单机运行
./bin/ChatServer

# Docker集群部署
docker-compose up -d
```

### 测试
```bash
# 编译测试客户端
make MavlinkTestClient

# 运行测试（模拟无人机）
./bin/MavlinkTestClient
```

## 项目结构

```
muduo_server/
├── src/                    # 源代码
│   ├── server/            # 服务器核心代码
│   │   ├── chatserver.cpp # 主服务器（协议识别）
│   │   └── mavlink_codec.cpp # MAVLink编解码器
│   └── client/            # 客户端代码
├── include/               # 头文件
├── test/                  # 测试代码
│   └── mavlink_test_client.cpp # 无人机模拟器
├── thirdparty/           # 第三方库
│   └── mavlink/          # MAVLink库
├── docker-compose.yml    # Docker编排
└── nginx.conf           # 负载均衡配置
```

## 支持的MAVLink消息

- **HEARTBEAT**: 心跳包，监控无人机在线状态
- **GPS_RAW_INT**: GPS位置信息
- **ATTITUDE**: 姿态数据（横滚、俯仰、偏航）
- **可扩展**: 易于添加更多消息类型支持

## 集群部署架构

- **Nginx负载均衡**: 将无人机连接分散到不同服务器节点
- **Redis数据同步**: 跨服务器实时数据同步，确保任何节点都能看到所有无人机状态
- **高可用设计**: 单节点故障不影响整体服务
