/**
 * proc-mem-inject - 运行时内存注入工具
 * 
 * 特点：
 * - 不修改任何文件（绕过文件完整性检测）
 * - 不使用 ptrace（绕过 ptrace 检测）
 * - 纯内存操作，进程重启后痕迹消失
 * - 反检测：时序随机化、进程伪装、分块访问
 * 
 * 用法:
 *   stealth_mem -k <root_key> -p <pid> -a <addr> -c <shellcode_file>
 *   stealth_mem -k <root_key> -p <pid> --hook <target_addr> -c <hook_code_file>
 *   stealth_mem -k <root_key> -p <pid> --read <addr> -s <size>
 *   stealth_mem -k <root_key> -p <pid> --write <addr> -d <hex_data>
 *   stealth_mem -k <root_key> -p <pid> --maps
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <csignal>
#include "memory_injector.h"
#include "anti_detect.h"
#include "patch_manager.h"

// 保存原始 argv 用于伪装
static int g_argc = 0;
static char** g_argv = nullptr;
static volatile bool g_running = true;

void signal_handler(int sig) {
    g_running = false;
    printf("\n[*] 收到信号 %d，正在退出...\n", sig);
}

void print_usage(const char* prog) {
    printf("proc-mem-inject v2.1 - 运行时内存注入 (反检测增强版)\n\n");
    printf("用法:\n");
    printf("  %s -k <root_key> -p <pid> [操作] [选项]\n\n", prog);
    printf("操作:\n");
    printf("  --maps                    显示内存映射\n");
    printf("  --read <addr> -s <size>   读取内存\n");
    printf("  --write <addr> -d <hex>   写入内存 (hex格式如: 1F2003D5)\n");
    printf("  --inject -c <file>        注入 shellcode 文件\n");
    printf("  --hook <addr> -c <file>   在指定地址安装 hook\n");
    printf("  --find <module>           查找模块基址\n");
    printf("  --batch <config>          批量应用 patch 配置文件\n");
    printf("  --monitor <config>        监控模式：检测还原并自动重新 patch\n\n");
    printf("反检测选项:\n");
    printf("  --stealth                 启用全部反检测 (默认)\n");
    printf("  --no-stealth              禁用反检测\n");
    printf("  --delay <min> <max>       设置延迟范围 (微秒, 默认 100-5000)\n");
    printf("  --chunk <size>            设置分块大小 (默认 64)\n");
    printf("  --fake-name <name>        伪装进程名 (默认 kworker/0:0)\n\n");
    printf("示例:\n");
    printf("  # 批量应用 patch\n");
    printf("  %s -k \"key\" -p 1234 --batch /data/local/tmp/wzry.patch\n\n", prog);
    printf("  # 监控模式 (Ctrl+C 退出)\n");
    printf("  %s -k \"key\" -p 1234 --monitor /data/local/tmp/wzry.patch\n\n", prog);
    printf("  # 查看内存映射\n");
    printf("  %s -k \"key\" -p 1234 --maps\n", prog);
}

// 十六进制字符串转字节
std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i + 1 < hex.length(); i += 2) {
        uint8_t byte = (uint8_t)strtol(hex.substr(i, 2).c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}


// 字节转十六进制字符串
std::string bytes_to_hex(const uint8_t* data, size_t size) {
    std::string hex;
    char buf[4];
    for (size_t i = 0; i < size; i++) {
        snprintf(buf, sizeof(buf), "%02X", data[i]);
        hex += buf;
        if ((i + 1) % 16 == 0) hex += "\n";
        else if ((i + 1) % 4 == 0) hex += " ";
    }
    return hex;
}

// 读取文件
std::vector<uint8_t> read_file(const std::string& path) {
    std::vector<uint8_t> data;
    std::ifstream file(path, std::ios::binary);
    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        data.resize(size);
        file.read((char*)data.data(), size);
    }
    return data;
}

int main(int argc, char* argv[]) {
    // 保存原始参数用于伪装
    g_argc = argc;
    g_argv = argv;
    
    std::string root_key;
    pid_t pid = 0;
    std::string operation;
    uint64_t addr = 0;
    size_t size = 0;
    std::string hex_data;
    std::string file_path;
    std::string module_name;
    
    // 反检测配置
    stealth::anti::AntiDetectConfig anti_config;
    bool stealth_enabled = true;

    // 解析参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            root_key = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            pid = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--maps") == 0) {
            operation = "maps";
        } else if (strcmp(argv[i], "--read") == 0 && i + 1 < argc) {
            operation = "read";
            addr = strtoull(argv[++i], nullptr, 0);
        } else if (strcmp(argv[i], "--write") == 0 && i + 1 < argc) {
            operation = "write";
            addr = strtoull(argv[++i], nullptr, 0);
        } else if (strcmp(argv[i], "--inject") == 0) {
            operation = "inject";
        } else if (strcmp(argv[i], "--hook") == 0 && i + 1 < argc) {
            operation = "hook";
            addr = strtoull(argv[++i], nullptr, 0);
        } else if (strcmp(argv[i], "--find") == 0 && i + 1 < argc) {
            operation = "find";
            module_name = argv[++i];
        } else if (strcmp(argv[i], "--batch") == 0 && i + 1 < argc) {
            operation = "batch";
            file_path = argv[++i];
        } else if (strcmp(argv[i], "--monitor") == 0 && i + 1 < argc) {
            operation = "monitor";
            file_path = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            hex_data = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            file_path = argv[++i];
        } else if (strcmp(argv[i], "--stealth") == 0) {
            stealth_enabled = true;
        } else if (strcmp(argv[i], "--no-stealth") == 0) {
            stealth_enabled = false;
        } else if (strcmp(argv[i], "--delay") == 0 && i + 2 < argc) {
            anti_config.min_delay_us = atoi(argv[++i]);
            anti_config.max_delay_us = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--chunk") == 0 && i + 1 < argc) {
            anti_config.chunk_size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--fake-name") == 0 && i + 1 < argc) {
            anti_config.fake_process_name = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    // 检查参数
    if (root_key.empty() || pid == 0 || operation.empty()) {
        print_usage(argv[0]);
        return 1;
    }
    
    // 初始化反检测
    if (stealth_enabled) {
        anti_config.enable_timing_jitter = true;
        anti_config.enable_process_mask = true;
        anti_config.enable_chunked_access = true;
        anti_config.enable_access_shuffle = true;
    } else {
        anti_config.enable_timing_jitter = false;
        anti_config.enable_process_mask = false;
        anti_config.enable_chunked_access = false;
        anti_config.enable_access_shuffle = false;
    }
    stealth::anti::init_anti_detect(g_argc, g_argv, anti_config);
    
    if (stealth_enabled) {
        // 静默模式，减少输出
    } else {
        printf("[*] 反检测: 已禁用\n");
    }

    // 创建注入器
    stealth::MemoryInjector injector(root_key);
    
    printf("[*] 附加到进程 %d...\n", pid);
    auto result = injector.attach(pid);
    if (result != stealth::MemInjectResult::SUCCESS) {
        printf("[-] 附加失败: %s\n", stealth::result_to_string(result));
        return 1;
    }
    printf("[+] 附加成功\n");

    // 执行操作
    if (operation == "maps") {
        std::vector<stealth::MemoryRegion> regions;
        if (injector.get_memory_maps(regions)) {
            printf("\n内存映射 (%zu 个区域):\n", regions.size());
            printf("%-18s %-18s %-5s %s\n", "Start", "End", "Perm", "Name");
            printf("--------------------------------------------------------------\n");
            for (const auto& r : regions) {
                char perms[4] = {
                    r.readable ? 'r' : '-',
                    r.writable ? 'w' : '-',
                    r.executable ? 'x' : '-',
                    '\0'
                };
                printf("0x%016lx 0x%016lx %s  %s\n", 
                       r.start, r.end, perms, r.name.c_str());
            }
        }
    }
    else if (operation == "read") {
        if (size == 0) size = 64;
        std::vector<uint8_t> buf(size);
        result = injector.read_memory(addr, buf.data(), size);
        if (result == stealth::MemInjectResult::SUCCESS) {
            printf("\n读取 0x%lx (%zu 字节):\n", addr, size);
            printf("%s\n", bytes_to_hex(buf.data(), size).c_str());
        } else {
            printf("[-] 读取失败: %s\n", stealth::result_to_string(result));
        }
    }
    else if (operation == "write") {
        if (hex_data.empty()) {
            printf("[-] 缺少数据 (-d)\n");
            return 1;
        }
        auto bytes = hex_to_bytes(hex_data);
        result = injector.write_memory(addr, bytes.data(), bytes.size());
        if (result == stealth::MemInjectResult::SUCCESS) {
            printf("[+] 写入成功: %zu 字节 @ 0x%lx\n", bytes.size(), addr);
        } else {
            printf("[-] 写入失败: %s\n", stealth::result_to_string(result));
        }
    }
    else if (operation == "find") {
        uint64_t base = injector.find_module_base(module_name);
        if (base != 0) {
            printf("[+] %s 基址: 0x%lx\n", module_name.c_str(), base);
        } else {
            printf("[-] 未找到模块: %s\n", module_name.c_str());
        }
    }
    else if (operation == "inject") {
        if (file_path.empty()) {
            printf("[-] 缺少 shellcode 文件 (-c)\n");
            return 1;
        }
        auto shellcode = read_file(file_path);
        if (shellcode.empty()) {
            printf("[-] 无法读取文件: %s\n", file_path.c_str());
            return 1;
        }
        result = injector.inject_shellcode(shellcode.data(), shellcode.size());
        if (result == stealth::MemInjectResult::SUCCESS) {
            printf("[+] Shellcode 注入成功 (%zu 字节)\n", shellcode.size());
        } else {
            printf("[-] 注入失败: %s\n", stealth::result_to_string(result));
        }
    }
    else if (operation == "hook") {
        if (file_path.empty()) {
            printf("[-] 缺少 hook 代码文件 (-c)\n");
            return 1;
        }
        auto hook_code = read_file(file_path);
        if (hook_code.empty()) {
            printf("[-] 无法读取文件: %s\n", file_path.c_str());
            return 1;
        }
        stealth::HookInfo info;
        result = injector.install_hook(addr, hook_code.data(), hook_code.size(), info);
        if (result == stealth::MemInjectResult::SUCCESS) {
            printf("[+] Hook 安装成功!\n");
            printf("    目标地址: 0x%lx\n", info.target_addr);
            printf("    Hook 地址: 0x%lx\n", info.hook_addr);
            printf("    跳板地址: 0x%lx\n", info.trampoline_addr);
        } else {
            printf("[-] Hook 失败: %s\n", stealth::result_to_string(result));
        }
    }
    else if (operation == "batch" || operation == "monitor") {
        // 批量 patch / 监控模式
        if (file_path.empty()) {
            printf("[-] 缺少配置文件\n");
            return 1;
        }
        
        stealth::PatchConfig config;
        if (!config.load(file_path)) {
            printf("[-] 无法加载配置文件: %s\n", file_path.c_str());
            return 1;
        }
        
        printf("[*] 加载了 %zu 个 patch 项\n", config.entries.size());
        
        // 解析模块基址
        for (auto& entry : config.entries) {
            if (!entry.enabled) {
                printf("[~] 跳过 (已禁用): %s\n", entry.name.c_str());
                continue;
            }
            
            uint64_t base = injector.find_module_base(entry.module);
            if (base == 0) {
                printf("[-] 找不到模块 %s，跳过: %s\n", entry.module.c_str(), entry.name.c_str());
                continue;
            }
            
            entry.resolved_addr = base + entry.offset;
            printf("[*] %s: %s + 0x%lx = 0x%lx\n", 
                   entry.name.c_str(), entry.module.c_str(), entry.offset, entry.resolved_addr);
        }
        
        // 应用所有 patch
        int success_count = 0;
        int fail_count = 0;
        
        for (auto& entry : config.entries) {
            if (!entry.enabled || entry.resolved_addr == 0) continue;
            
            result = injector.write_memory(entry.resolved_addr, 
                                           entry.patched.data(), 
                                           entry.patched.size());
            if (result == stealth::MemInjectResult::SUCCESS) {
                entry.applied = true;
                success_count++;
                printf("[+] 应用成功: %s\n", entry.name.c_str());
            } else {
                fail_count++;
                printf("[-] 应用失败: %s (%s)\n", entry.name.c_str(), stealth::result_to_string(result));
            }
        }
        
        printf("\n[*] 批量 patch 完成: %d 成功, %d 失败\n", success_count, fail_count);
        
        // 监控模式
        if (operation == "monitor" && success_count > 0) {
            printf("[*] 进入监控模式，按 Ctrl+C 退出...\n");
            signal(SIGINT, signal_handler);
            signal(SIGTERM, signal_handler);
            
            uint32_t check_count = 0;
            uint32_t reapply_count = 0;
            
            while (g_running) {
                check_count++;
                
                for (auto& entry : config.entries) {
                    if (!entry.enabled || !entry.applied || entry.resolved_addr == 0) continue;
                    
                    // 读取当前内存
                    std::vector<uint8_t> current(entry.patched.size());
                    result = injector.read_memory(entry.resolved_addr, current.data(), current.size());
                    
                    if (result == stealth::MemInjectResult::SUCCESS) {
                        // 检查是否被还原
                        bool reverted = (current != entry.patched);
                        
                        if (reverted) {
                            printf("[!] 检测到还原: %s，重新应用...\n", entry.name.c_str());
                            result = injector.write_memory(entry.resolved_addr,
                                                           entry.patched.data(),
                                                           entry.patched.size());
                            if (result == stealth::MemInjectResult::SUCCESS) {
                                reapply_count++;
                                printf("[+] 重新应用成功\n");
                            } else {
                                printf("[-] 重新应用失败\n");
                            }
                        }
                    }
                }
                
                // 每秒检查一次
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                // 每 60 秒输出状态
                if (check_count % 60 == 0) {
                    printf("[*] 监控中... 检查 %u 次, 重新应用 %u 次\n", check_count, reapply_count);
                }
            }
            
            printf("[*] 监控结束，共检查 %u 次，重新应用 %u 次\n", check_count, reapply_count);
        }
    }

    injector.detach();
    return 0;
}
