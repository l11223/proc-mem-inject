/**
 * 基于 rwProcMem33 的内核级无痕内存注入器实现
 */

#include "kernel_memory_injector.h"
#include "rwprocmem33/rwprocmem33_wrapper.h"
#include <memory>
#include <algorithm>
#include <cstring>
#include <unistd.h>

namespace stealth {

// rwProcMem33 接口适配层
class RwProcMem33Adapter {
public:
    RwProcMem33Adapter() = default;
    ~RwProcMem33Adapter() = default;
    
    // 驱动连接
    int connect_driver(const std::string& auth_key) {
        return m_driver.connect(auth_key);
    }
    
    void disconnect_driver() {
        m_driver.disconnect();
    }
    
    bool is_connected() const {
        return m_driver.is_connected();
    }
    
    // 驱动隐藏
    bool hide_kernel_module() {
        return m_driver.hide_kernel_module();
    }
    
    // 进程操作
    uint64_t open_process(pid_t pid) {
        return m_driver.open_process(pid);
    }
    
    bool close_process(uint64_t handle) {
        return m_driver.close_process(handle);
    }
    
    // 权限提升
    bool set_process_root(uint64_t handle) {
        return m_driver.set_process_root(handle);
    }
    
    // 内存操作
    bool read_process_memory(uint64_t handle, uint64_t addr, void* buf, size_t size, size_t* bytes_read, bool force_read) {
        return m_driver.read_process_memory(handle, addr, buf, size, bytes_read, force_read);
    }
    
    bool write_process_memory(uint64_t handle, uint64_t addr, const void* buf, size_t size, size_t* bytes_written, bool force_write) {
        return m_driver.write_process_memory(handle, addr, buf, size, bytes_written, force_write);
    }
    
    // 内存映射
    bool get_memory_maps(uint64_t handle, bool physical_only, std::vector<KernelMemoryRegion>& regions) {
        std::vector<DRIVER_REGION_INFO> driver_regions;
        if (!m_driver.get_memory_maps(handle, physical_only, driver_regions)) {
            return false;
        }
        
        // 转换格式
        for (const auto& dr : driver_regions) {
            KernelMemoryRegion region;
            region.start = dr.baseaddress;
            region.end = dr.baseaddress + dr.size;
            region.size = dr.size;
            region.protection = dr.protection;
            region.type = dr.type;
            region.name = dr.name;
            regions.push_back(region);
        }
        
        return true;
    }
    
    bool check_memory_valid(uint64_t handle, uint64_t addr) {
        return m_driver.check_memory_valid(handle, addr);
    }
    
    // 进程信息
    bool get_process_cmdline(uint64_t handle, std::string& cmdline) {
        return m_driver.get_process_cmdline(handle, cmdline);
    }
    
    bool get_physical_memory_size(uint64_t handle, uint64_t& size_kb) {
        return m_driver.get_physical_memory_size(handle, size_kb);
    }

private:
    rwprocmem33::RwProcMem33Driver m_driver;
};

// KernelMemoryInjector 的私有实现
class KernelMemoryInjector::Impl {
public:
    Impl() : m_process_handle(0) {}
    
    ~Impl() {
        detach_process();
        disconnect_driver();
    }
    
    KernelInjectResult connect_driver(const std::string& auth_key) {
        std::string key = auth_key.empty() ? generate_default_auth_key() : auth_key;
        
        int result = m_adapter.connect_driver(key);
        if (result < 0) {
            return KernelInjectResult::ERR_DRIVER_CONNECT_FAILED;
        }
        
        return KernelInjectResult::SUCCESS;
    }
    
    void disconnect_driver() {
        m_adapter.disconnect_driver();
    }
    
    bool is_driver_connected() const {
        return m_adapter.is_connected();
    }
    
    KernelInjectResult hide_driver() {
        if (!m_adapter.is_connected()) {
            return KernelInjectResult::ERR_DRIVER_NOT_CONNECTED;
        }
        
        if (!m_adapter.hide_kernel_module()) {
            return KernelInjectResult::ERR_DRIVER_HIDE_FAILED;
        }
        
        return KernelInjectResult::SUCCESS;
    }
    
    KernelInjectResult attach_process(pid_t pid) {
        if (!m_adapter.is_connected()) {
            return KernelInjectResult::ERR_DRIVER_NOT_CONNECTED;
        }
        
        m_process_handle = m_adapter.open_process(pid);
        if (m_process_handle == 0) {
            return KernelInjectResult::ERR_PROCESS_OPEN_FAILED;
        }
        
        m_target_pid = pid;
        return KernelInjectResult::SUCCESS;
    }
    
    void detach_process() {
        if (m_process_handle != 0) {
            m_adapter.close_process(m_process_handle);
            m_process_handle = 0;
            m_target_pid = 0;
        }
    }
    
    KernelInjectResult elevate_process_to_root() {
        if (m_process_handle == 0) {
            return KernelInjectResult::ERR_PROCESS_NOT_FOUND;
        }
        
        if (!m_adapter.set_process_root(m_process_handle)) {
            return KernelInjectResult::ERR_PROCESS_OPEN_FAILED;
        }
        
        return KernelInjectResult::SUCCESS;
    }
    
    KernelInjectResult read_memory(uint64_t addr, void* buf, size_t size, bool force_read) {
        if (m_process_handle == 0) {
            return KernelInjectResult::ERR_PROCESS_NOT_FOUND;
        }
        
        size_t bytes_read = 0;
        if (!m_adapter.read_process_memory(m_process_handle, addr, buf, size, &bytes_read, force_read)) {
            return KernelInjectResult::ERR_MEMORY_READ_FAILED;
        }
        
        return KernelInjectResult::SUCCESS;
    }
    
    KernelInjectResult write_memory(uint64_t addr, const void* buf, size_t size, bool force_write) {
        if (m_process_handle == 0) {
            return KernelInjectResult::ERR_PROCESS_NOT_FOUND;
        }
        
        size_t bytes_written = 0;
        if (!m_adapter.write_process_memory(m_process_handle, addr, buf, size, &bytes_written, force_write)) {
            return KernelInjectResult::ERR_MEMORY_WRITE_FAILED;
        }
        
        return KernelInjectResult::SUCCESS;
    }
    
    KernelInjectResult get_memory_regions(std::vector<KernelMemoryRegion>& regions, bool physical_only) {
        if (m_process_handle == 0) {
            return KernelInjectResult::ERR_PROCESS_NOT_FOUND;
        }
        
        if (!m_adapter.get_memory_maps(m_process_handle, physical_only, regions)) {
            return KernelInjectResult::ERR_MEMORY_READ_FAILED;
        }
        
        return KernelInjectResult::SUCCESS;
    }
    
    bool is_memory_valid(uint64_t addr) {
        if (m_process_handle == 0) {
            return false;
        }
        
        return m_adapter.check_memory_valid(m_process_handle, addr);
    }
    
    KernelInjectResult get_process_cmdline(std::string& cmdline) {
        if (m_process_handle == 0) {
            return KernelInjectResult::ERR_PROCESS_NOT_FOUND;
        }
        
        if (!m_adapter.get_process_cmdline(m_process_handle, cmdline)) {
            return KernelInjectResult::ERR_MEMORY_READ_FAILED;
        }
        
        return KernelInjectResult::SUCCESS;
    }
    
    KernelInjectResult get_physical_memory_size(uint64_t& size_kb) {
        if (m_process_handle == 0) {
            return KernelInjectResult::ERR_PROCESS_NOT_FOUND;
        }
        
        if (!m_adapter.get_physical_memory_size(m_process_handle, size_kb)) {
            return KernelInjectResult::ERR_MEMORY_READ_FAILED;
        }
        
        return KernelInjectResult::SUCCESS;
    }
    
    KernelInjectResult inject_shellcode(const uint8_t* shellcode, size_t size, uint64_t& injected_addr) {
        // 1. 找到可执行空间
        injected_addr = find_executable_space(size);
        if (injected_addr == 0) {
            return KernelInjectResult::ERR_NO_EXECUTABLE_SPACE;
        }
        
        // 2. 写入 shellcode
        auto result = write_memory(injected_addr, shellcode, size, true);
        if (result != KernelInjectResult::SUCCESS) {
            return result;
        }
        
        return KernelInjectResult::SUCCESS;
    }
    
    KernelInjectResult install_hook(uint64_t target_addr, const uint8_t* hook_code, size_t hook_size, KernelHookInfo& hook_info) {
        // 1. 读取并备份原始指令
        hook_info.target_addr = target_addr;
        hook_info.orig_size = arm64::calculate_instruction_size(nullptr, 16);
        
        auto result = read_memory(target_addr, hook_info.orig_bytes, hook_info.orig_size, false);
        if (result != KernelInjectResult::SUCCESS) {
            return result;
        }
        
        // 2. 分配空间存放 hook 代码和跳板
        size_t total_size = hook_size + 64;  // hook 代码 + 跳板
        hook_info.hook_addr = find_executable_space(total_size);
        if (hook_info.hook_addr == 0) {
            return KernelInjectResult::ERR_NO_EXECUTABLE_SPACE;
        }
        
        hook_info.trampoline_addr = hook_info.hook_addr + hook_size;
        
        // 3. 写入 hook 代码
        result = write_memory(hook_info.hook_addr, hook_code, hook_size, true);
        if (result != KernelInjectResult::SUCCESS) {
            return result;
        }
        
        // 4. 生成并写入跳板
        uint8_t trampoline[64];
        size_t trampoline_size;
        arm64::generate_trampoline(hook_info.orig_bytes, hook_info.orig_size,
                                   target_addr + hook_info.orig_size,
                                   trampoline, &trampoline_size);
        
        result = write_memory(hook_info.trampoline_addr, trampoline, trampoline_size, true);
        if (result != KernelInjectResult::SUCCESS) {
            return result;
        }
        
        // 5. 生成跳转指令并覆盖目标函数入口
        uint8_t jump_code[16];
        size_t jump_size;
        arm64::generate_jump_instruction(target_addr, hook_info.hook_addr, jump_code, &jump_size);
        
        result = write_memory(target_addr, jump_code, jump_size, true);
        if (result != KernelInjectResult::SUCCESS) {
            return result;
        }
        
        hook_info.active = true;
        return KernelInjectResult::SUCCESS;
    }
    
    KernelInjectResult remove_hook(const KernelHookInfo& hook_info) {
        if (!hook_info.active) {
            return KernelInjectResult::SUCCESS;
        }
        
        // 恢复原始指令
        return write_memory(hook_info.target_addr, hook_info.orig_bytes, hook_info.orig_size, true);
    }
    
    uint64_t find_module_base(const std::string& module_name) {
        std::vector<KernelMemoryRegion> regions;
        if (get_memory_regions(regions, false) != KernelInjectResult::SUCCESS) {
            return 0;
        }
        
        for (const auto& region : regions) {
            if (region.name.find(module_name) != std::string::npos) {
                return region.start;
            }
        }
        
        return 0;
    }
    
    uint64_t find_executable_space(size_t required_size) {
        std::vector<KernelMemoryRegion> regions;
        if (get_memory_regions(regions, false) != KernelInjectResult::SUCCESS) {
            return 0;
        }
        
        // 在可执行区域末尾找空闲空间
        for (const auto& region : regions) {
            if (!region.executable() || region.name.empty()) continue;
            
            // 检查区域末尾是否有足够的空间
            size_t check_size = std::min((size_t)4096, (size_t)region.size);
            uint64_t check_addr = region.start + region.size - check_size;
            
            std::vector<uint8_t> buf(check_size);
            if (read_memory(check_addr, buf.data(), check_size, false) != KernelInjectResult::SUCCESS) {
                continue;
            }
            
            // 从后往前找连续的 0x00
            size_t zero_count = 0;
            for (size_t i = check_size; i > 0; i--) {
                if (buf[i-1] == 0x00) {
                    zero_count++;
                    if (zero_count >= required_size + 16) {
                        return check_addr + i - 1;
                    }
                } else {
                    zero_count = 0;
                }
            }
        }
        
        return 0;
    }

private:
    std::string generate_default_auth_key() {
        // rwProcMem33 的默认隐蔽通信密钥
        return "e84523d7b60d5d341a7c4d1861773ecd";
    }

private:
    RwProcMem33Adapter m_adapter;
    uint64_t m_process_handle;
    pid_t m_target_pid;
};

// KernelMemoryInjector 公共接口实现
KernelMemoryInjector::KernelMemoryInjector() : m_impl(new Impl()) {}

KernelMemoryInjector::~KernelMemoryInjector() {
    delete m_impl;
}

KernelInjectResult KernelMemoryInjector::connect_driver(const std::string& auth_key) {
    return m_impl->connect_driver(auth_key);
}

void KernelMemoryInjector::disconnect_driver() {
    m_impl->disconnect_driver();
}

bool KernelMemoryInjector::is_driver_connected() const {
    return m_impl->is_driver_connected();
}

KernelInjectResult KernelMemoryInjector::hide_driver() {
    return m_impl->hide_driver();
}

KernelInjectResult KernelMemoryInjector::attach_process(pid_t pid) {
    return m_impl->attach_process(pid);
}

void KernelMemoryInjector::detach_process() {
    m_impl->detach_process();
}

KernelInjectResult KernelMemoryInjector::elevate_process_to_root() {
    return m_impl->elevate_process_to_root();
}

KernelInjectResult KernelMemoryInjector::read_memory(uint64_t addr, void* buf, size_t size, bool force_read) {
    return m_impl->read_memory(addr, buf, size, force_read);
}

KernelInjectResult KernelMemoryInjector::write_memory(uint64_t addr, const void* buf, size_t size, bool force_write) {
    return m_impl->write_memory(addr, buf, size, force_write);
}

KernelInjectResult KernelMemoryInjector::get_memory_regions(std::vector<KernelMemoryRegion>& regions, bool physical_only) {
    return m_impl->get_memory_regions(regions, physical_only);
}

bool KernelMemoryInjector::is_memory_valid(uint64_t addr) {
    return m_impl->is_memory_valid(addr);
}

KernelInjectResult KernelMemoryInjector::get_process_cmdline(std::string& cmdline) {
    return m_impl->get_process_cmdline(cmdline);
}

KernelInjectResult KernelMemoryInjector::get_physical_memory_size(uint64_t& size_kb) {
    return m_impl->get_physical_memory_size(size_kb);
}

KernelInjectResult KernelMemoryInjector::inject_shellcode(const uint8_t* shellcode, size_t size, uint64_t& injected_addr) {
    return m_impl->inject_shellcode(shellcode, size, injected_addr);
}

KernelInjectResult KernelMemoryInjector::install_hook(uint64_t target_addr, const uint8_t* hook_code, size_t hook_size, KernelHookInfo& hook_info) {
    return m_impl->install_hook(target_addr, hook_code, hook_size, hook_info);
}

KernelInjectResult KernelMemoryInjector::remove_hook(const KernelHookInfo& hook_info) {
    return m_impl->remove_hook(hook_info);
}

uint64_t KernelMemoryInjector::find_module_base(const std::string& module_name) {
    return m_impl->find_module_base(module_name);
}

uint64_t KernelMemoryInjector::find_executable_space(size_t required_size) {
    return m_impl->find_executable_space(required_size);
}

const char* KernelMemoryInjector::result_to_string(KernelInjectResult result) {
    switch (result) {
        case KernelInjectResult::SUCCESS: return "SUCCESS";
        case KernelInjectResult::ERR_DRIVER_NOT_CONNECTED: return "ERR_DRIVER_NOT_CONNECTED";
        case KernelInjectResult::ERR_DRIVER_CONNECT_FAILED: return "ERR_DRIVER_CONNECT_FAILED";
        case KernelInjectResult::ERR_PROCESS_NOT_FOUND: return "ERR_PROCESS_NOT_FOUND";
        case KernelInjectResult::ERR_PROCESS_OPEN_FAILED: return "ERR_PROCESS_OPEN_FAILED";
        case KernelInjectResult::ERR_MEMORY_READ_FAILED: return "ERR_MEMORY_READ_FAILED";
        case KernelInjectResult::ERR_MEMORY_WRITE_FAILED: return "ERR_MEMORY_WRITE_FAILED";
        case KernelInjectResult::ERR_NO_EXECUTABLE_SPACE: return "ERR_NO_EXECUTABLE_SPACE";
        case KernelInjectResult::ERR_HOOK_INSTALL_FAILED: return "ERR_HOOK_INSTALL_FAILED";
        case KernelInjectResult::ERR_DRIVER_HIDE_FAILED: return "ERR_DRIVER_HIDE_FAILED";
        default: return "UNKNOWN";
    }
}

// ARM64 汇编辅助函数实现
namespace arm64 {

void generate_jump_instruction(uint64_t from, uint64_t to, uint8_t* out, size_t* out_size) {
    int64_t offset = to - from;
    
    if (offset >= -0x8000000 && offset < 0x8000000) {
        // 使用 B 指令（相对跳转，±128MB）
        uint32_t insn = 0x14000000 | ((offset >> 2) & 0x3FFFFFF);
        memcpy(out, &insn, 4);
        *out_size = 4;
    } else {
        // 使用 LDR + BR（远跳转）
        uint32_t ldr = 0x58000050;  // LDR X16, #8
        uint32_t br = 0xD61F0200;   // BR X16
        
        memcpy(out, &ldr, 4);
        memcpy(out + 4, &br, 4);
        memcpy(out + 8, &to, 8);
        *out_size = 16;
    }
}

void generate_trampoline(const uint8_t* orig_bytes, size_t orig_size, uint64_t return_addr, uint8_t* out, size_t* out_size) {
    // 跳板 = 原始指令 + 跳转回原函数
    memcpy(out, orig_bytes, orig_size);
    
    size_t jump_size;
    generate_jump_instruction(0, return_addr, out + orig_size, &jump_size);
    
    *out_size = orig_size + jump_size;
}

size_t calculate_instruction_size(const uint8_t* code, size_t min_size) {
    // ARM64 指令固定 4 字节
    // 我们需要至少 16 字节来放置远跳转
    size_t size = 0;
    while (size < min_size || size < 16) {
        size += 4;
    }
    return size;
}

} // namespace arm64

} // namespace stealth