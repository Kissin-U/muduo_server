# 基于Muduo网络库的无人机通信服务器

一个高性能的无人机通信服务器，支持MAVLink和JSON双协议智能识别与处理。

## 项目特点

- **双协议支持**: 自动识别并处理MAVLink和JSON协议
- **高性能网络**: 基于Muduo网络库，支持大量并发连接
- **分布式部署**: 集成Redis发布/订阅，支持集群部署
- **线程安全**: 完善的并发控制机制

## 技术栈

- **网络库**: Muduo (基于Reactor模式)
- **协议**: MAVLink v2.0 + JSON
- **缓存**: Redis (发布/订阅)
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

# Docker部署
docker-compose up -d
```

## 项目结构

```
muduo_server/
├── src/                    # 源代码
│   ├── server/            # 服务器核心代码
│   └── client/            # 客户端代码
├── include/               # 头文件
├── thirdparty/           # 第三方库
├── docker-compose.yml    # Docker编排
└── nginx.conf           # 负载均衡配置
```

## 协议支持

- **MAVLink**: 心跳、GPS、姿态等核心消息
- **JSON**: 地面站控制指令和状态查询

## 部署架构

支持Nginx TCP负载均衡的集群部署，实现高可用和水平扩展。
