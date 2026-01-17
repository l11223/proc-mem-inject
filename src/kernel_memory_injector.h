/**
 * 基于 rwProcMem33 的内核级无痕内存注入器
 * 
 * 核心优势：
 * - 真正的内核级操作，无用户态痕迹
 * - 硬件级读写进程内存
 * - 驱动级隐藏机制
 * - Android 15 完美支持
 */

#ifndef KERNEL_MEMORY_INJECTOR_H
#define KERNEL_MEMORY_INJECTOR_H

#include <string>
#include <vector>
#include <cstdint>

namespace stealth {

// 基于 rwProcMem33 的内存区域信息
struct KernelMemoryRegion {
    uint64_t start;
    uint64_t end;
    uint64_t size;
    uint32_t protection;
    uint32_t type;
    std::string name;
    
    bool readable() const { return protection & 0x01; }
    bool writable() const { return protection & 0x02; }
    bool executable() const { return protection & 0x04; }
};

// 内核级注入结果
enum class KernelInjectResult {
    SUCCESS = 0,
    ERR_DRIVER_NOT_CONNECTED,
    ERR_DRIVER_CONNECT_FAILED,
    ERR_PROCESS_NOT_FOUND,
    ERR_PROCESS_OPEN_FAILED,
    ERR_MEMORY_READ_FAILED,
    ERR_MEMORY_WRITE_FAILED,
    ERR_NO_EXECUTABLE_SPACE,
    ERR_HOOK_INSTALL_FAILED,
    ERR_DRIVER_HIDE_FAILED
};

// Hook 信息
struct KernelHookInfo {
    uint64_t target_addr;       // 目标函数地址
    uint64_t hook_addr;         // Hook 代码地址
    uint64_t trampoline_addr;   // 跳板地址
    uint8_t orig_bytes[32];     // 原始字节
    size_t orig_size;           // 原始大小
    bool active;                // 是否激活
};

/**
 * 内核级无痕内存注入器
 * 基于 rwProcMem33 驱动实现真正的内核级操作
 */
class KernelMemoryInjector {
public:
    KernelMemoryInjector();
    ~KernelMemoryInjector();

    // 驱动连接管理
    KernelInjectResult connect_driver(const std::string& auth_key = "");
    void disconnect_driver();
    bool is_driver_connected() const;
    
    // 驱动隐藏 (反检测核心功能)
    KernelInjectResult hide_driver();
    
    // 进程操作
    KernelInjectResult attach_process(pid_t pid);
    void detach_process();
    
    // 权限提升
    KernelInjectResult elevate_process_to_root();
    
    // 内存操作 (内核级，无用户态痕迹)
    KernelInjectResult read_memory(uint64_t addr, void* buf, size_t size, bool force_read = false);
    KernelInjectResult write_memory(uint64_t addr, const void* buf, size_t size, bool force_write = false);
    
    // 内存映射查询
    KernelInjectResult get_memory_regions(std::vector<KernelMemoryRegion>& regions, bool physical_only = false);
    bool is_memory_valid(uint64_t addr);
    
    // 进程信息
    KernelInjectResult get_process_cmdline(std::string& cmdline);
    KernelInjectResult get_physical_memory_size(uint64_t& size_kb);
    
    // 高级注入功能
    KernelInjectResult inject_shellcode(const uint8_t* shellcode, size_t size, uint64_t& injected_addr);
    KernelInjectResult install_hook(uint64_t target_addr, const uint8_t* hook_code, size_t hook_size, KernelHookInfo& hook_info);
    KernelInjectResult remove_hook(const KernelHookInfo& hook_info);
    
    // 内存搜索和修改
    KernelInjectResult search_memory(const uint8_t* pattern, size_t pattern_size, std::vector<uint64_t>& results);
    KernelInjectResult patch_memory(uint64_t addr, const uint8_t* patch_data, size_t size);
    
    // 工具函数
    uint64_t find_module_base(const std::string& module_name);
    uint64_t find_executable_space(size_t required_size);
    
    // 错误处理
    static const char* result_to_string(KernelInjectResult result);

private:
    class Impl;
    Impl* m_impl;
    
    // 禁用拷贝
    KernelMemoryInjector(const KernelMemoryInjector&) = delete;
    KernelMemoryInjector& operator=(const KernelMemoryInjector&) = delete;
};

// ARM64 汇编辅助函数
namespace arm64 {
    void generate_jump_instruction(uint64_t from, uint64_t to, uint8_t* out, size_t* out_size);
    void generate_trampoline(const uint8_t* orig_bytes, size_t orig_size, uint64_t return_addr, uint8_t* out, size_t* out_size);
    size_t calculate_instruction_size(const uint8_t* code, size_t min_size);
}

} // namespace stealth

#endif // KERNEL_MEMORY_INJECTOR_H