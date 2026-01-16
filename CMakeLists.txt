# proc-mem-inject - 运行时内存注入工具
# 通过 /proc/pid/mem 直接读写目标进程内存
# 不修改文件，不用 ptrace

cmake_minimum_required(VERSION 3.18.1)
project("proc-mem-inject")

set(CMAKE_CXX_STANDARD 17)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/outputs/${ANDROID_ABI})

# SKRoot Lite kernel_root_kit 路径 (本地 deps 目录)
set(SKROOT_KIT_DIR "${CMAKE_SOURCE_DIR}/deps/kernel_root_kit")

# 源文件
add_executable(stealth_mem
    src/main.cpp
    src/memory_injector.cpp
)

# 头文件
target_include_directories(stealth_mem PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include
    ${SKROOT_KIT_DIR}/include
)

# 链接 SKRoot Lite 静态库
target_link_libraries(stealth_mem
    ${SKROOT_KIT_DIR}/lib/libkernel_root_kit_static.a
    log
    z
)
