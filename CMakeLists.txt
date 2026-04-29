cmake_minimum_required(VERSION 3.15)
project(SuOS VERSION 1.0.0 LANGUAGES CXX)

# 1. 关键设置：导出编译数据库供 clangd 使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 2. C++ 标准设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 3. 编译选项（适配 ARM 交叉编译）
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 4. 查找依赖（Boost 会根据 SYSROOT 自动寻找）
find_package(Boost REQUIRED COMPONENTS system)

# 5. 定义项目目录变量
set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core")
set(UDS_DIR "${CORE_DIR}/suUDS")
set(UDS_BASIC_DIR "${CORE_DIR}/suUDS/basic")
set(UDS_MGR_DIR "${CORE_DIR}/suUDS/suUdsMgr")
set(UDS_MSG_DIR "${CORE_DIR}/suUDS/suUdsMsg")
set(RUNTIME_DIR "${CORE_DIR}/suRuntime")

# --- 库配置开始 ---

# [基础库] 包含 Uds_Client.h
add_library(suUDS_basic STATIC
    "${UDS_BASIC_DIR}/src/Uds_Client.cpp"
    "${UDS_BASIC_DIR}/src/Uds_Server.cpp"
)
target_include_directories(suUDS_basic PUBLIC "${UDS_BASIC_DIR}/include")
target_link_libraries(suUDS_basic PUBLIC Boost::system)

# [管理器库] 只有头文件
# 建议：如果 clangd 找不到头文件，可以将 INTERFACE 改为 STATIC 并添加一个空的 stub.cpp
add_library(suUDS_mgr INTERFACE)
target_include_directories(suUDS_mgr INTERFACE "${UDS_MGR_DIR}")
target_link_libraries(suUDS_mgr INTERFACE suUDS_basic)

# [消息库] 只有头文件
add_library(suUDS_msg INTERFACE)
target_include_directories(suUDS_msg INTERFACE "${UDS_MSG_DIR}")

# [运行时库]
add_library(suRuntime INTERFACE)
target_include_directories(suRuntime INTERFACE "${RUNTIME_DIR}/include")

# --- 主程序配置 ---

add_executable(SuOS
    SuOS.cpp
    # 将头文件显式列出，有助于部分 IDE/插件 建立索引
    "${UDS_MGR_DIR}/Uds_Client_heartbeat.hpp"
    "${UDS_MGR_DIR}/Uds_ClientMgr.hpp"
    core/SysService/stopCause.h
)

target_link_libraries(SuOS PRIVATE
    suUDS_basic
    suUDS_mgr
    suUDS_msg
    suRuntime
    pthread
)