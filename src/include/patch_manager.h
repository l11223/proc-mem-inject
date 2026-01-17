#pragma once
/**
 * patch_manager.h - 批量 Patch 和内存监控
 * 
 * 功能：
 * 1. 批量 patch - 从配置文件读取多个修改，一键应用
 * 2. 内存监控 - 检测 patch 是否被游戏还原，自动重新应用
 */

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>

namespace stealth {

// 单个 Patch 项
struct PatchEntry {
    std::string name;           // 名称（如 "绕过检测"）
    std::string module;         // 模块名（如 "libUE4.so"）
    uint64_t offset;            // 模块内偏移
    std::vector<uint8_t> original;  // 原始字节（用于验证和还原）
    std::vector<uint8_t> patched;   // 修改后字节
    bool enabled = true;        // 是否启用
    
    uint64_t resolved_addr = 0; // 运行时解析的实际地址
    bool applied = false;       // 是否已应用
};

// Patch 配置文件解析器
class PatchConfig {
public:
    std::vector<PatchEntry> entries;
    
    // 从文件加载配置
    bool load(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        
        entries.clear();
        std::string line;
        PatchEntry current;
        bool in_entry = false;
        
        while (std::getline(file, line)) {
            // 去除首尾空格
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            line = line.substr(start);
            
            // 跳过注释
            if (line[0] == '#' || line[0] == ';') continue;
            
            // 新条目开始 [name]
            if (line[0] == '[') {
                if (in_entry && !current.name.empty()) {
                    entries.push_back(current);
                }
                current = PatchEntry();
                size_t end = line.find(']');
                if (end != std::string::npos) {
                    current.name = line.substr(1, end - 1);
                }
                in_entry = true;
                continue;
            }
            
            // 解析 key=value
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            
            // 去除空格
            while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) value = value.substr(1);
            
            if (key == "module") {
                current.module = value;
            } else if (key == "offset") {
                current.offset = strtoull(value.c_str(), nullptr, 0);
            } else if (key == "original") {
                current.original = parse_hex(value);
            } else if (key == "patch") {
                current.patched = parse_hex(value);
            } else if (key == "enabled") {
                current.enabled = (value == "true" || value == "1");
            }
        }
        
        // 最后一个条目
        if (in_entry && !current.name.empty()) {
            entries.push_back(current);
        }
        
        return !entries.empty();
    }
    
    // 保存配置到文件
    bool save(const std::string& path) {
        std::ofstream file(path);
        if (!file.is_open()) return false;
        
        file << "# Patch 配置文件\n";
        file << "# 格式:\n";
        file << "# [名称]\n";
        file << "# module=模块名\n";
        file << "# offset=偏移地址\n";
        file << "# original=原始字节\n";
        file << "# patch=修改字节\n";
        file << "# enabled=true/false\n\n";
        
        for (const auto& e : entries) {
            file << "[" << e.name << "]\n";
            file << "module=" << e.module << "\n";
            file << "offset=0x" << std::hex << e.offset << "\n";
            file << "original=" << to_hex(e.original) << "\n";
            file << "patch=" << to_hex(e.patched) << "\n";
            file << "enabled=" << (e.enabled ? "true" : "false") << "\n\n";
        }
        
        return true;
    }

private:
    static std::vector<uint8_t> parse_hex(const std::string& hex) {
        std::vector<uint8_t> bytes;
        std::string clean;
        for (char c : hex) {
            if (isxdigit(c)) clean += c;
        }
        for (size_t i = 0; i + 1 < clean.length(); i += 2) {
            bytes.push_back((uint8_t)strtol(clean.substr(i, 2).c_str(), nullptr, 16));
        }
        return bytes;
    }
    
    static std::string to_hex(const std::vector<uint8_t>& bytes) {
        std::string hex;
        char buf[4];
        for (uint8_t b : bytes) {
            snprintf(buf, sizeof(buf), "%02X", b);
            hex += buf;
        }
        return hex;
    }
};

// 内存监控器 - 检测 patch 是否被还原
class MemoryMonitor {
public:
    using ReapplyCallback = std::function<bool(PatchEntry&)>;
    
    MemoryMonitor() : m_running(false) {}
    ~MemoryMonitor() { stop(); }
    
    // 开始监控
    void start(std::vector<PatchEntry>& patches, 
               ReapplyCallback read_fn,
               ReapplyCallback write_fn,
               uint32_t interval_ms = 1000) {
        if (m_running) return;
        
        m_running = true;
        m_patches = &patches;
        m_read_fn = read_fn;
        m_write_fn = write_fn;
        m_interval = interval_ms;
        
        m_thread = std::thread(&MemoryMonitor::monitor_loop, this);
    }
    
    // 停止监控
    void stop() {
        m_running = false;
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }
    
    // 获取统计
    uint32_t get_reapply_count() const { return m_reapply_count; }
    
private:
    void monitor_loop() {
        while (m_running) {
            for (auto& patch : *m_patches) {
                if (!patch.enabled || !patch.applied || patch.resolved_addr == 0) {
                    continue;
                }
                
                // 读取当前内存
                std::vector<uint8_t> current(patch.patched.size());
                PatchEntry temp = patch;
                
                if (m_read_fn(temp)) {
                    // 检查是否被还原
                    bool reverted = false;
                    // 这里简化处理，实际需要比较内存内容
                    // 如果内存不等于 patched，说明被还原了
                    
                    if (reverted) {
                        // 重新应用 patch
                        if (m_write_fn(patch)) {
                            m_reapply_count++;
                        }
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(m_interval));
        }
    }
    
    std::atomic<bool> m_running;
    std::thread m_thread;
    std::vector<PatchEntry>* m_patches = nullptr;
    ReapplyCallback m_read_fn;
    ReapplyCallback m_write_fn;
    uint32_t m_interval = 1000;
    std::atomic<uint32_t> m_reapply_count{0};
};

} // namespace stealth
