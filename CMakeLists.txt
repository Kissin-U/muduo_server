cmake_minimum_required(VERSION 3.0)
project(chat)

# 配置编译选项

set(CMAKE_CXX_FLAGES ${CMAKE_CXX_FLAGES} -g)

# 配置最终的可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 执行CMAKE命令的目录
set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)

# 配置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/mavlink)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/mavlink/common)

# 加载子目录
add_subdirectory(src)
add_subdirectory(test)
