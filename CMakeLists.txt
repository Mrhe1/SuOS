cmake_minimum_required(VERSION 3.15)
project(SuOS VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# 编译选项
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 查找依赖库
find_package(Boost REQUIRED COMPONENTS system)

# 定义项目目录变量
set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core")
set(UDS_BASIC_DIR "${CORE_DIR}/suUDS/basic")
set(UDS_MGR_DIR "${CORE_DIR}/suUDS/suUdsMgr")
set(UDS_MSG_DIR "${CORE_DIR}/suUDS/suUdsMsg")
set(NETWORK_DIR "${CORE_DIR}/Network_service")
set(RUNTIME_DIR "${CORE_DIR}/suRuntime")
set(MAIN_DIR "${CORE_DIR}/Main")

# 创建 UDS 基础库
add_library(suUDS_basic STATIC
    "${UDS_BASIC_DIR}/src/Uds_Client.cpp"
    "${UDS_BASIC_DIR}/src/Uds_Server.cpp"
)

target_include_directories(suUDS_basic PUBLIC
    "${UDS_BASIC_DIR}/include"
)

target_link_libraries(suUDS_basic PUBLIC
    Boost::system
)

# 创建 UDS 管理器库 (只有头文件，设为 INTERFACE)
add_library(suUDS_mgr INTERFACE)

# 注意：INTERFACE 库的所有命令都必须使用 INTERFACE 关键字
target_include_directories(suUDS_mgr INTERFACE
    "${UDS_MGR_DIR}"
)

target_link_libraries(suUDS_mgr INTERFACE
    suUDS_basic
)

# 创建 UDS 消息库
add_library(suUDS_msg INTERFACE)

target_include_directories(suUDS_msg INTERFACE
    "${UDS_MSG_DIR}"
)

# 创建网络服务库
add_library(suNetwork STATIC
    # 如果有源文件，在这里添加
     "${NETWORK_DIR}/Connectors/mqtt_service/bin/mqtt_client.cpp"
)

target_include_directories(suNetwork PUBLIC
    "${NETWORK_DIR}/Connectors/base"
)

target_link_libraries(suNetwork PUBLIC
    Boost::system
)

# 创建运行时库
add_library(suRuntime INTERFACE)

target_include_directories(suRuntime INTERFACE
    "${RUNTIME_DIR}/include"
)

# 创建主程序可执行文件
add_executable(SuOS
    SuOS.cpp
)

target_include_directories(SuOS PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${MAIN_DIR}/Database"
    "${MAIN_DIR}/Router"
    "${MAIN_DIR}/include"
)

target_link_libraries(SuOS PRIVATE
    suUDS_basic
    suUDS_mgr
    suUDS_msg
    suNetwork
    suRuntime
    pthread
)

# 创建测试可执行文件（可选）
if(EXISTS "${UDS_BASIC_DIR}/src/test_uds.cpp")    
    add_executable(uds_test
        "${UDS_BASIC_DIR}/src/test_uds.cpp"
    )

    target_link_libraries(uds_test PRIVATE
        suUDS_basic
        suUDS_mgr
        suUDS_msg
        pthread
    )
endif()

# 输出配置信息
message(STATUS "SuOS Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")