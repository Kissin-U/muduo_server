# 编译MAVLink测试客户端
add_executable(MavlinkTestClient mavlink_test_client.cpp)

# 链接必要的库
target_link_libraries(MavlinkTestClient pthread)
