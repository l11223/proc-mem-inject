#pragma once
/**
 * proc-mem-inject - 运行时内存注入
 * 
 * 原理：
 * 1. 通过 SKRoot 获取 ROOT 权限
 * 2. 直接读写 /proc/<pid>/mem 操作目标进程内存
 * 3. 不修改任何文件，纯内存操作
 * 4. 进程重启后痕迹自动消失
 */

#include <string>
#include <vector>
#include <cstdint>
#include <sys/types.h>

namespace stealth {

// 内存区域信息
struct MemoryRegion {
    uint64_t start;
    uint64_t end;
    bool readable;
    bool writable;
    bool executable;
    std::string name;
};

// Hook 信息
struct HookInfo {
    uint64_t target_addr;      // 目标函数地址
    uint64_t hook_addr;        // Hook 函数地址
    uint64_t trampoline_addr;  // 跳板地址（调用原函数）
    uint8_t orig_bytes[16];    // 备份的原始字节
    uint32_t orig_size;        // 备份大小
    bool active;
};

// 注入结果
enum class MemInjectResult {
    SUCCESS = 0,
    ERR_NO_ROOT,
    ERR_OPEN_MEM,
    ERR_READ_MEM,
    ERR_WRITE_MEM,
    ERR_NO_SPACE,
    ERR_INVALID_ADDR,
    ERR_HOOK_FAILED,
};

const char* result_to_string(MemInjectResult result);

class MemoryInjector {
public:
    MemoryInjector(const std::string& root_key);
    ~MemoryInjector();

    // 附加到目标进程
    MemInjectResult attach(pid_t pid);
    
    // 分离
    void detach();

    // 读取内存
    MemInjectResult read_memory(uint64_t addr, void* buf, size_t size);
    
    // 写入内存
    MemInjectResult write_memory(uint64_t addr, const void* buf, size_t size);

    // 获取内存映射
    bool get_memory_maps(std::vector<MemoryRegion>& regions);

    // 查找模块基址
    uint64_t find_module_base(const std::string& module_name);

    // 在目标进程中分配内存（找可写可执行区域）
    uint64_t allocate_memory(size_t size);

    // 安装 inline hook
    MemInjectResult install_hook(uint64_t target_addr, const uint8_t* hook_code, 
                                  size_t hook_size, HookInfo& out_info);

    // 移除 hook
    MemInjectResult remove_hook(const HookInfo& info);

    // 直接写入 shellcode 并执行
    MemInjectResult inject_shellcode(const uint8_t* shellcode, size_t size);

private:
    std::string m_root_key;
    pid_t m_target_pid;
    int m_mem_fd;
    std::vector<MemoryRegion> m_regions;
    
    // 内部辅助函数
    bool ensure_root();
    bool open_mem();
    void close_mem();
    uint64_t find_code_cave(size_t size);  // 找代码空洞
};

// ARM64 汇编辅助
namespace arm64 {
    // 生成跳转指令
    void generate_jump(uint64_t from, uint64_t to, uint8_t* out, size_t* out_size);
    
    // 生成 Hook 跳板
    void generate_trampoline(const uint8_t* orig_bytes, size_t orig_size,
                             uint64_t return_addr, uint8_t* out, size_t* out_size);
    
    // 计算需要备份的指令大小（确保不破坏指令边界）
    size_t calculate_backup_size(const uint8_t* code, size_t min_size);
}

} // namespace stealth
