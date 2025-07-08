# 基于Ubuntu 20.04构建无人机集群电力巡检服务器
FROM ubuntu:20.04

# 设置环境变量避免交互式安装
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# 安装系统依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libboost-all-dev \
    libmysqlclient-dev \
    libhiredis-dev \
    libssl-dev \
    zlib1g-dev \
    libjsoncpp-dev \
    tzdata \
    && rm -rf /var/lib/apt/lists/*

# 设置时区
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# 创建应用目录
WORKDIR /app

# 复制源代码
COPY . .

# 创建构建目录并编译项目
RUN mkdir -p build && cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=11 \
    && make -j$(nproc)

# 创建日志目录
RUN mkdir -p /app/logs

# 创建运行用户
RUN useradd -r -s /bin/false uavserver && \
    chown -R uavserver:uavserver /app

# 暴露端口
EXPOSE 6000 6001 8080

# 设置运行用户
USER uavserver

# 健康检查
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# 启动服务
CMD ["./build/bin/ChatServer"]
