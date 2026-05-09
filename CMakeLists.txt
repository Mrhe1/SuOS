cmake_minimum_required(VERSION 3.15)
project(SuOS VERSION 1.0.0 LANGUAGES CXX)

# 强制开启编译数据库支持，让 clangd 能正确读取
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++ 标准及配置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 确保在 Debug/Release 下都有良好的编译输出
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 基础目录定义
set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core")

# --- 1. 定义核心模块 (INTERFACE 库仅用于传播头文件路径) ---

# [UDS 相关模块]
add_library(lib_uds_basic STATIC 
    "${CORE_DIR}/suUDS/basic/src/Uds_Client.cpp"
    "${CORE_DIR}/suUDS/basic/src/Uds_Server.cpp"
)
target_include_directories(lib_uds_basic PUBLIC "${CORE_DIR}/suUDS/basic/include")

add_library(lib_uds_mgr INTERFACE)
target_include_directories(lib_uds_mgr INTERFACE "${CORE_DIR}/suUDS/suUdsMgr")

add_library(lib_uds_msg INTERFACE)
target_include_directories(lib_uds_msg INTERFACE "${CORE_DIR}/suUDS/suUdsMsg")

# [Graphics 模块]：解决你的 VramProvider 找不到的问题
add_library(lib_graphics INTERFACE)
target_include_directories(lib_graphics INTERFACE 
    "${CORE_DIR}/Graphics/Rga/include"
    "${CORE_DIR}/Graphics/Vram"
    "${CORE_DIR}/Graphics/Vtimer"
)

# [其他核心模块]
add_library(lib_runtime INTERFACE)
target_include_directories(lib_runtime INTERFACE "${CORE_DIR}/suRuntime/include")

add_library(lib_main_inc INTERFACE)
target_include_directories(lib_main_inc INTERFACE "${CORE_DIR}/Main/include")

# --- 2. 主程序配置 ---

add_executable(SuOS 
    SuOS.cpp 
    core/Main/src/main.cpp
)

# 自动关联所有依赖，头文件搜索路径会自动传播
target_link_libraries(SuOS PRIVATE
    lib_uds_basic
    lib_uds_mgr
    lib_uds_msg
    lib_graphics
    lib_runtime
    lib_main_inc
    pthread
)

# --- 3. 可选：如果还需要包含根目录 ---
target_include_directories(SuOS PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")