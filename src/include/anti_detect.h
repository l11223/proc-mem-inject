#pragma once
/**
 * anti_detect.h - 反检测模块
 * 
 * 功能：
 * 1. 时序随机化 - 避免固定访问模式被检测
 * 2. 进程伪装 - 修改进程名/cmdline
 * 3. 分块访问 - 避免大块连续读写
 * 4. 访问混淆 - 随机化访问顺序
 */

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/prctl.h>
#include <cstring>

namespace stealth {
namespace anti {

// 反检测配置
struct AntiDetectConfig {
    bool enable_timing_jitter = true;      // 启用时序抖动
    bool enable_process_mask = true;       // 启用进程伪装
    bool enable_chunked_access = true;     // 启用分块访问
    bool enable_access_shuffle = true;     // 启用访问顺序混淆
    
    uint32_t min_delay_us = 100;           // 最小延迟 (微秒)
    uint32_t max_delay_us = 5000;          // 最大延迟 (微秒)
    size_t chunk_size = 64;                // 分块大小
    
    std::string fake_process_name = "kworker/0:0";  // 伪装进程名
};

// 全局配置
inline AntiDetectConfig g_config;

// 随机数生成器
class SecureRandom {
public:
    static SecureRandom& instance() {
        static SecureRandom inst;
        return inst;
    }
    
    uint32_t next_u32() {
        return m_dist(m_gen);
    }
    
    uint32_t next_u32(uint32_t min, uint32_t max) {
        std::uniform_int_distribution<uint32_t> dist(min, max);
        return dist(m_gen);
    }
    
    double next_double() {
        return m_double_dist(m_gen);
    }

private:
    SecureRandom() : m_gen(std::random_device{}()), 
                     m_dist(0, UINT32_MAX),
                     m_double_dist(0.0, 1.0) {}
    
    std::mt19937 m_gen;
    std::uniform_int_distribution<uint32_t> m_dist;
    std::uniform_real_distribution<double> m_double_dist;
};

// 时序随机化 - 在操作之间加入随机延迟
inline void timing_jitter() {
    if (!g_config.enable_timing_jitter) return;
    
    uint32_t delay = SecureRandom::instance().next_u32(
        g_config.min_delay_us, g_config.max_delay_us);
    
    std::this_thread::sleep_for(std::chrono::microseconds(delay));
}

// 进程名伪装
inline bool mask_process_name(const char* fake_name) {
    if (!g_config.enable_process_mask) return true;
    
    // 方法1: prctl 修改进程名 (最多15字符)
    if (prctl(PR_SET_NAME, fake_name, 0, 0, 0) != 0) {
        return false;
    }
    
    return true;
}

// 修改 cmdline (需要在 main 开始时调用)
inline void mask_cmdline(int argc, char** argv, const char* fake_cmdline) {
    if (!g_config.enable_process_mask) return;
    if (argc < 1 || argv == nullptr) return;
    
    // 计算原始 argv 总长度
    size_t total_len = 0;
    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
            total_len += strlen(argv[i]) + 1;
        }
    }
    
    // 清空并写入假的 cmdline
    size_t fake_len = strlen(fake_cmdline);
    if (fake_len < total_len) {
        memset(argv[0], 0, total_len);
        memcpy(argv[0], fake_cmdline, fake_len);
    }
}

// 分块读取 - 将大块读取拆分成小块，每块之间加延迟
template<typename ReadFunc>
inline bool chunked_read(uint64_t addr, void* buf, size_t size, ReadFunc read_fn) {
    if (!g_config.enable_chunked_access || size <= g_config.chunk_size) {
        return read_fn(addr, buf, size);
    }
    
    uint8_t* ptr = static_cast<uint8_t*>(buf);
    size_t remaining = size;
    uint64_t current_addr = addr;
    
    while (remaining > 0) {
        size_t chunk = std::min(remaining, g_config.chunk_size);
        
        // 随机化实际读取大小 (在 chunk/2 到 chunk 之间)
        if (g_config.enable_access_shuffle && chunk > 16) {
            chunk = SecureRandom::instance().next_u32(chunk / 2, chunk);
            chunk = (chunk + 3) & ~3;  // 4字节对齐
        }
        
        if (!read_fn(current_addr, ptr, chunk)) {
            return false;
        }
        
        ptr += chunk;
        current_addr += chunk;
        remaining -= chunk;
        
        // 块之间加延迟
        timing_jitter();
    }
    
    return true;
}

// 分块写入
template<typename WriteFunc>
inline bool chunked_write(uint64_t addr, const void* buf, size_t size, WriteFunc write_fn) {
    if (!g_config.enable_chunked_access || size <= g_config.chunk_size) {
        return write_fn(addr, buf, size);
    }
    
    const uint8_t* ptr = static_cast<const uint8_t*>(buf);
    size_t remaining = size;
    uint64_t current_addr = addr;
    
    while (remaining > 0) {
        size_t chunk = std::min(remaining, g_config.chunk_size);
        
        if (!write_fn(current_addr, ptr, chunk)) {
            return false;
        }
        
        ptr += chunk;
        current_addr += chunk;
        remaining -= chunk;
        
        timing_jitter();
    }
    
    return true;
}

// 混淆访问顺序 - 对多个地址的访问进行随机排序
inline void shuffle_addresses(std::vector<uint64_t>& addrs) {
    if (!g_config.enable_access_shuffle) return;
    
    auto& rng = SecureRandom::instance();
    for (size_t i = addrs.size() - 1; i > 0; i--) {
        size_t j = rng.next_u32(0, i);
        std::swap(addrs[i], addrs[j]);
    }
}

// 初始化反检测 (在 main 开始时调用)
inline void init_anti_detect(int argc, char** argv, const AntiDetectConfig& config = {}) {
    g_config = config;
    
    // 伪装进程名
    mask_process_name(g_config.fake_process_name.c_str());
    mask_cmdline(argc, argv, g_config.fake_process_name.c_str());
}

// 获取当前配置
inline AntiDetectConfig& get_config() {
    return g_config;
}

} // namespace anti
} // namespace stealth
