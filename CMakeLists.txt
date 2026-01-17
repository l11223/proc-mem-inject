# proc-mem-inject - 运行时内存注入工具
# 支持两种模式：
# 1. 传统模式：通过 /proc/pid/mem 直接读写 (stealth_mem)
# 2. 内核模式：基于 rwProcMem33 驱动的内核级操作 (kernel_mem)

cmake_minimum_required(VERSION 3.18.1)
project("proc-mem-inject")

set(CMAKE_CXX_STANDARD 17)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/outputs/${ANDROID_ABI})

# SKRoot Lite kernel_root_kit 路径 (本地 deps 目录)
set(SKROOT_KIT_DIR "${CMAKE_SOURCE_DIR}/deps/kernel_root_kit")

# rwProcMem33 路径
set(RWPROCMEM33_DIR "${CMAKE_SOURCE_DIR}/deps/rwProcMem33")

# 传统版本 - 基于 /proc/pid/mem
add_executable(stealth_mem
    src/main.cpp
    src/memory_injector.cpp
)

# 内核版本 - 基于 rwProcMem33 驱动
add_executable(kernel_mem
    src/kernel_main.cpp
    src/kernel_memory_injector.cpp
)

# 传统版本头文件
target_include_directories(stealth_mem PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include
    ${SKROOT_KIT_DIR}/include
)

# 内核版本头文件
target_include_directories(kernel_mem PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include
    ${RWPROCMEM33_DIR}/rwProcMem33Module/testKo/jni
)

# 传统版本链接库
target_link_libraries(stealth_mem
    ${SKROOT_KIT_DIR}/lib/libkernel_root_kit_static.a
    log
    z
)

# 内核版本链接库
target_link_libraries(kernel_mem
    log
    z
)
