/**
 * proc-mem-inject - 运行时内存注入实现
 * 
 * 核心原理：
 * - 使用 /proc/<pid>/mem 直接读写目标进程内存
 * - 需要 ROOT 权限（通过 SKRoot Lite 获取）
 * - 不使用 ptrace，不修改文件
 */

#include "memory_injector.h"
#include "rootkit_umbrella.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace stealth {

const char* result_to_string(MemInjectResult result) {
    switch (result) {
        case MemInjectResult::SUCCESS: return "SUCCESS";
        case MemInjectResult::ERR_NO_ROOT: return "ERR_NO_ROOT";
        case MemInjectResult::ERR_OPEN_MEM: return "ERR_OPEN_MEM";
        case MemInjectResult::ERR_READ_MEM: return "ERR_READ_MEM";
        case MemInjectResult::ERR_WRITE_MEM: return "ERR_WRITE_MEM";
        case MemInjectResult::ERR_NO_SPACE: return "ERR_NO_SPACE";
        case MemInjectResult::ERR_INVALID_ADDR: return "ERR_INVALID_ADDR";
        case MemInjectResult::ERR_HOOK_FAILED: return "ERR_HOOK_FAILED";
        default: return "UNKNOWN";
    }
}

MemoryInjector::MemoryInjector(const std::string& root_key)
    : m_root_key(root_key), m_target_pid(0), m_mem_fd(-1) {
}

MemoryInjector::~MemoryInjector() {
    detach();
}

bool MemoryInjector::ensure_root() {
    auto err = kernel_root::get_root(m_root_key.c_str());
    return kernel_root::is_ok(err);
}

bool MemoryInjector::open_mem() {
    if (m_mem_fd >= 0) return true;
    
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/mem", m_target_pid);
    
    // 以读写模式打开
    m_mem_fd = open(path, O_RDWR);
    if (m_mem_fd < 0) {
        // 尝试只读
        m_mem_fd = open(path, O_RDONLY);
    }
    
    return m_mem_fd >= 0;
}

void MemoryInjector::close_mem() {
    if (m_mem_fd >= 0) {
        close(m_mem_fd);
        m_mem_fd = -1;
    }
}


MemInjectResult MemoryInjector::attach(pid_t pid) {
    if (!ensure_root()) {
        return MemInjectResult::ERR_NO_ROOT;
    }
    
    m_target_pid = pid;
    
    if (!open_mem()) {
        return MemInjectResult::ERR_OPEN_MEM;
    }
    
    // 读取内存映射
    get_memory_maps(m_regions);
    
    return MemInjectResult::SUCCESS;
}

void MemoryInjector::detach() {
    close_mem();
    m_target_pid = 0;
    m_regions.clear();
}

MemInjectResult MemoryInjector::read_memory(uint64_t addr, void* buf, size_t size) {
    if (m_mem_fd < 0) return MemInjectResult::ERR_OPEN_MEM;
    
    if (lseek64(m_mem_fd, addr, SEEK_SET) == (off64_t)-1) {
        return MemInjectResult::ERR_INVALID_ADDR;
    }
    
    ssize_t ret = read(m_mem_fd, buf, size);
    if (ret != (ssize_t)size) {
        return MemInjectResult::ERR_READ_MEM;
    }
    
    return MemInjectResult::SUCCESS;
}

MemInjectResult MemoryInjector::write_memory(uint64_t addr, const void* buf, size_t size) {
    if (m_mem_fd < 0) return MemInjectResult::ERR_OPEN_MEM;
    
    // 重新以写模式打开
    close_mem();
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/mem", m_target_pid);
    m_mem_fd = open(path, O_RDWR);
    if (m_mem_fd < 0) {
        return MemInjectResult::ERR_OPEN_MEM;
    }
    
    if (lseek64(m_mem_fd, addr, SEEK_SET) == (off64_t)-1) {
        return MemInjectResult::ERR_INVALID_ADDR;
    }
    
    ssize_t ret = write(m_mem_fd, buf, size);
    if (ret != (ssize_t)size) {
        return MemInjectResult::ERR_WRITE_MEM;
    }
    
    return MemInjectResult::SUCCESS;
}

bool MemoryInjector::get_memory_maps(std::vector<MemoryRegion>& regions) {
    regions.clear();
    
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", m_target_pid);
    
    std::ifstream maps(path);
    if (!maps.is_open()) return false;
    
    std::string line;
    while (std::getline(maps, line)) {
        MemoryRegion region;
        
        // 解析格式: start-end perms offset dev inode pathname
        uint64_t start, end;
        char perms[5];
        
        if (sscanf(line.c_str(), "%lx-%lx %4s", &start, &end, perms) >= 3) {
            region.start = start;
            region.end = end;
            region.readable = (perms[0] == 'r');
            region.writable = (perms[1] == 'w');
            region.executable = (perms[2] == 'x');
            
            // 提取路径名
            size_t pos = line.find('/');
            if (pos != std::string::npos) {
                region.name = line.substr(pos);
            } else {
                pos = line.find('[');
                if (pos != std::string::npos) {
                    region.name = line.substr(pos);
                }
            }
            
            regions.push_back(region);
        }
    }
    
    return !regions.empty();
}

uint64_t MemoryInjector::find_module_base(const std::string& module_name) {
    for (const auto& region : m_regions) {
        if (region.name.find(module_name) != std::string::npos) {
            return region.start;
        }
    }
    return 0;
}

uint64_t MemoryInjector::find_code_cave(size_t size) {
    // 在可执行区域末尾找空闲空间
    // 通常 .text 段后面会有一些填充的 0x00 或 NOP
    
    for (const auto& region : m_regions) {
        if (!region.executable) continue;
        if (region.name.empty()) continue;
        
        // 读取区域末尾
        size_t check_size = std::min((size_t)4096, (size_t)(region.end - region.start));
        uint64_t check_addr = region.end - check_size;
        
        std::vector<uint8_t> buf(check_size);
        if (read_memory(check_addr, buf.data(), check_size) != MemInjectResult::SUCCESS) {
            continue;
        }
        
        // 从后往前找连续的 0x00 或 NOP (0xD503201F for ARM64)
        size_t zero_count = 0;
        for (size_t i = check_size; i > 0; i--) {
            if (buf[i-1] == 0x00) {
                zero_count++;
                if (zero_count >= size + 16) {
                    // 找到足够大的空洞
                    return check_addr + i - 1;
                }
            } else {
                zero_count = 0;
            }
        }
    }
    
    return 0;
}

uint64_t MemoryInjector::allocate_memory(size_t size) {
    // 方案1：找代码空洞
    uint64_t cave = find_code_cave(size);
    if (cave != 0) return cave;
    
    // 方案2：在匿名映射区域找空间
    for (const auto& region : m_regions) {
        if (region.name.find("[anon:") != std::string::npos && 
            region.writable && region.executable) {
            return region.start;
        }
    }
    
    return 0;
}


MemInjectResult MemoryInjector::install_hook(uint64_t target_addr, 
                                              const uint8_t* hook_code,
                                              size_t hook_size,
                                              HookInfo& out_info) {
    // 1. 读取并备份原始指令
    out_info.target_addr = target_addr;
    out_info.orig_size = arm64::calculate_backup_size(nullptr, 16);
    
    auto ret = read_memory(target_addr, out_info.orig_bytes, out_info.orig_size);
    if (ret != MemInjectResult::SUCCESS) return ret;
    
    // 2. 分配空间存放 hook 代码和跳板
    size_t total_size = hook_size + 64;  // hook 代码 + 跳板
    uint64_t alloc_addr = allocate_memory(total_size);
    if (alloc_addr == 0) return MemInjectResult::ERR_NO_SPACE;
    
    out_info.hook_addr = alloc_addr;
    out_info.trampoline_addr = alloc_addr + hook_size;
    
    // 3. 写入 hook 代码
    ret = write_memory(alloc_addr, hook_code, hook_size);
    if (ret != MemInjectResult::SUCCESS) return ret;
    
    // 4. 生成并写入跳板（原始指令 + 跳回）
    uint8_t trampoline[64];
    size_t trampoline_size;
    arm64::generate_trampoline(out_info.orig_bytes, out_info.orig_size,
                               target_addr + out_info.orig_size,
                               trampoline, &trampoline_size);
    
    ret = write_memory(out_info.trampoline_addr, trampoline, trampoline_size);
    if (ret != MemInjectResult::SUCCESS) return ret;
    
    // 5. 生成跳转指令并覆盖目标函数入口
    uint8_t jump_code[16];
    size_t jump_size;
    arm64::generate_jump(target_addr, out_info.hook_addr, jump_code, &jump_size);
    
    ret = write_memory(target_addr, jump_code, jump_size);
    if (ret != MemInjectResult::SUCCESS) return ret;
    
    out_info.active = true;
    return MemInjectResult::SUCCESS;
}

MemInjectResult MemoryInjector::remove_hook(const HookInfo& info) {
    if (!info.active) return MemInjectResult::SUCCESS;
    
    // 恢复原始指令
    return write_memory(info.target_addr, info.orig_bytes, info.orig_size);
}

MemInjectResult MemoryInjector::inject_shellcode(const uint8_t* shellcode, size_t size) {
    uint64_t addr = allocate_memory(size);
    if (addr == 0) return MemInjectResult::ERR_NO_SPACE;
    
    return write_memory(addr, shellcode, size);
}

// ARM64 汇编辅助实现
namespace arm64 {

void generate_jump(uint64_t from, uint64_t to, uint8_t* out, size_t* out_size) {
    int64_t offset = to - from;
    
    if (offset >= -0x8000000 && offset < 0x8000000) {
        // 使用 B 指令（相对跳转，±128MB）
        uint32_t insn = 0x14000000 | ((offset >> 2) & 0x3FFFFFF);
        memcpy(out, &insn, 4);
        *out_size = 4;
    } else {
        // 使用 LDR + BR（远跳转）
        // LDR X16, [PC, #8]
        // BR X16
        // .quad target_addr
        uint32_t ldr = 0x58000050;  // LDR X16, #8
        uint32_t br = 0xD61F0200;   // BR X16
        
        memcpy(out, &ldr, 4);
        memcpy(out + 4, &br, 4);
        memcpy(out + 8, &to, 8);
        *out_size = 16;
    }
}

void generate_trampoline(const uint8_t* orig_bytes, size_t orig_size,
                         uint64_t return_addr, uint8_t* out, size_t* out_size) {
    // 跳板 = 原始指令 + 跳转回原函数
    memcpy(out, orig_bytes, orig_size);
    
    size_t jump_size;
    generate_jump(0, return_addr, out + orig_size, &jump_size);
    
    *out_size = orig_size + jump_size;
}

size_t calculate_backup_size(const uint8_t* code, size_t min_size) {
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
