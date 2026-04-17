# debian-toolchain.cmake
# 设置目标系统名称
set(CMAKE_SYSTEM_NAME Linux)
# 设置目标处理器架构 (根据实际 Debian 目标修改，如 aarch64, armhf, x86_64)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 指定交叉编译器的路径 (假设你安装了 mingw-w64 或专门的 cross-compiler)
# 例如：x86_64-linux-gnu-gcc
set(CMAKE_C_COMPILER /usr/bin/x86_64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/x86_64-linux-gnu-g++)

# 指定 Debian 的 sysroot 路径 (如果使用了 debootstrap 或 chroot 环境)
# set(CMAKE_FIND_ROOT_PATH /path/to/debian-sysroot)

# 搜索策略：只在 sysroot 中查找库和头文件，不在宿主系统中查找
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)