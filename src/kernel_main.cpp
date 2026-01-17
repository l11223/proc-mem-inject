/**
 * 基于 rwProcMem33 的内核级无痕注入工具
 * 
 * 特点：
 * - 真正的内核级操作，无用户态痕迹
 * - 硬件级读写进程内存
 * - 驱动级隐藏机制
 * - Android 15 完美支持
 * - 基于成熟的 rwProcMem33 驱动
 */

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <getopt.h>
#include "kernel_memory_injector.h"

using namespace stealth;

void print_banner() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                 🚀 Kernel Memory Injector v2.0              ║
║                   基于 rwProcMem33 驱动                      ║
╠══════════════════════════════════════════════════════════════╣
║  特点:                                                       ║
║  ✅ 真正的内核级操作，无用户态痕迹                           ║
║  ✅ 硬件级读写进程内存                                       ║
║  ✅ 驱动级隐藏机制                                           ║
║  ✅ Android 15 完美支持                                      ║
║  ✅ 基于成熟的 rwProcMem33 驱动                              ║
╚══════════════════════════════════════════════════════════════╝
)" << std::endl;
}

void print_usage(const char* prog) {
    std::cout << "用法: " << prog << " [选项]\n\n";
    std::cout << "选项:\n";
    std::cout << "  -k, --auth-key <key>     驱动认证密钥 (默认使用内置密钥)\n";
    std::cout << "  -p, --pid <pid>          目标进程 PID\n";
    std::cout << "  -a, --addr <addr>        内存地址 (十六进制)\n";
    std::cout << "  -s, --size <size>        数据大小\n";
    std::cout << "  -d, --data <hex>         十六进制数据\n";
    std::cout << "  -f, --file <path>        shellcode 文件路径\n";
    std::cout << "  --hide                   隐藏驱动\n";
    std::cout << "  --root                   提升目标进程权限到 root\n";
    std::cout << "  --force                  强制读写 (忽略权限检查)\n";
    std::cout << "\n操作:\n";
    std::cout << "  --list-processes         列出所有进程\n";
    std::cout << "  --list-modules           列出目标进程的模块\n";
    std::cout << "  --read                   读取内存\n";
    std::cout << "  --write                  写入内存\n";
    std::cout << "  --inject                 注入 shellcode\n";
    std::cout << "  --hook <target>          安装 hook\n";
    std::cout << "  --search <pattern>       搜索内存模式\n";
    std::cout << "\n示例:\n";
    std::cout << "  # 连接驱动并隐藏\n";
    std::cout << "  " << prog << " --hide\n\n";
    std::cout << "  # 读取进程内存\n";
    std::cout << "  " << prog << " -p 1234 --read -a 0x7000000000 -s 64\n\n";
    std::cout << "  # 写入内存数据\n";
    std::cout << "  " << prog << " -p 1234 --write -a 0x7000000000 -d 1F2003D5\n\n";
    std::cout << "  # 注入 shellcode\n";
    std::cout << "  " << prog << " -p 1234 --inject -f /data/local/tmp/hook.bin\n\n";
    std::cout << "  # 提升进程权限\n";
    std::cout << "  " << prog << " -p 1234 --root\n\n";
}

std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i + 1 < hex.length(); i += 2) {
        uint8_t byte = (uint8_t)strtol(hex.substr(i, 2).c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

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

std::vector<uint8_t> read_file(const std::string& path) {
    std::vector<uint8_t> data;
    FILE* file = fopen(path.c_str(), "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);
        data.resize(size);
        fread(data.data(), 1, size, file);
        fclose(file);
    }
    return data;
}

int main(int argc, char* argv[]) {
    print_banner();
    
    // 命令行参数
    std::string auth_key;
    pid_t target_pid = 0;
    uint64_t addr = 0;
    size_t size = 0;
    std::string hex_data;
    std::string file_path;
    uint64_t hook_target = 0;
    std::string search_pattern;
    bool force_mode = false;
    
    // 操作标志
    bool hide_driver = false;
    bool elevate_root = false;
    bool list_processes = false;
    bool list_modules = false;
    bool read_memory = false;
    bool write_memory = false;
    bool inject_shellcode = false;
    bool install_hook = false;
    bool search_memory = false;
    
    // 解析命令行参数
    static struct option long_options[] = {
        {"auth-key", required_argument, 0, 'k'},
        {"pid", required_argument, 0, 'p'},
        {"addr", required_argument, 0, 'a'},
        {"size", required_argument, 0, 's'},
        {"data", required_argument, 0, 'd'},
        {"file", required_argument, 0, 'f'},
        {"hide", no_argument, 0, 1001},
        {"root", no_argument, 0, 1002},
        {"force", no_argument, 0, 1003},
        {"list-processes", no_argument, 0, 1004},
        {"list-modules", no_argument, 0, 1005},
        {"read", no_argument, 0, 1006},
        {"write", no_argument, 0, 1007},
        {"inject", no_argument, 0, 1008},
        {"hook", required_argument, 0, 1009},
        {"search", required_argument, 0, 1010},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int c;
    while ((c = getopt_long(argc, argv, "k:p:a:s:d:f:h", long_options, nullptr)) != -1) {
        switch (c) {
            case 'k': auth_key = optarg; break;
            case 'p': target_pid = atoi(optarg); break;
            case 'a': addr = strtoull(optarg, nullptr, 0); break;
            case 's': size = atoi(optarg); break;
            case 'd': hex_data = optarg; break;
            case 'f': file_path = optarg; break;
            case 1001: hide_driver = true; break;
            case 1002: elevate_root = true; break;
            case 1003: force_mode = true; break;
            case 1004: list_processes = true; break;
            case 1005: list_modules = true; break;
            case 1006: read_memory = true; break;
            case 1007: write_memory = true; break;
            case 1008: inject_shellcode = true; break;
            case 1009: 
                install_hook = true;
                hook_target = strtoull(optarg, nullptr, 0);
                break;
            case 1010:
                search_memory = true;
                search_pattern = optarg;
                break;
            case 'h':
            default:
                print_usage(argv[0]);
                return 0;
        }
    }
    
    // 创建内核级注入器
    KernelMemoryInjector injector;
    
    std::cout << "[*] 连接内核驱动..." << std::endl;
    auto result = injector.connect_driver(auth_key);
    if (result != KernelInjectResult::SUCCESS) {
        std::cerr << "[-] 驱动连接失败: " << KernelMemoryInjector::result_to_string(result) << std::endl;
        std::cerr << "    请确保 rwProcMem33 驱动已正确加载" << std::endl;
        return 1;
    }
    std::cout << "[+] 驱动连接成功" << std::endl;
    
    // 隐藏驱动
    if (hide_driver) {
        std::cout << "[*] 隐藏内核驱动..." << std::endl;
        result = injector.hide_driver();
        if (result == KernelInjectResult::SUCCESS) {
            std::cout << "[+] 驱动隐藏成功" << std::endl;
        } else {
            std::cerr << "[-] 驱动隐藏失败: " << KernelMemoryInjector::result_to_string(result) << std::endl;
        }
    }
    
    // 需要进程操作的命令
    if (target_pid > 0) {
        std::cout << "[*] 附加到进程 " << target_pid << "..." << std::endl;
        result = injector.attach_process(target_pid);
        if (result != KernelInjectResult::SUCCESS) {
            std::cerr << "[-] 进程附加失败: " << KernelMemoryInjector::result_to_string(result) << std::endl;
            return 1;
        }
        std::cout << "[+] 进程附加成功" << std::endl;
        
        // 提升权限
        if (elevate_root) {
            std::cout << "[*] 提升进程权限到 root..." << std::endl;
            result = injector.elevate_process_to_root();
            if (result == KernelInjectResult::SUCCESS) {
                std::cout << "[+] 权限提升成功" << std::endl;
            } else {
                std::cerr << "[-] 权限提升失败: " << KernelMemoryInjector::result_to_string(result) << std::endl;
            }
        }
        
        // 列出模块
        if (list_modules) {
            std::cout << "[*] 获取进程内存映射..." << std::endl;
            std::vector<KernelMemoryRegion> regions;
            result = injector.get_memory_regions(regions, false);
            if (result == KernelInjectResult::SUCCESS) {
                std::cout << "\n内存映射 (" << regions.size() << " 个区域):" << std::endl;
                std::cout << "地址范围                    大小        权限  类型    名称" << std::endl;
                std::cout << "================================================================" << std::endl;
                for (const auto& region : regions) {
                    char perms[4] = {
                        region.readable() ? 'r' : '-',
                        region.writable() ? 'w' : '-',
                        region.executable() ? 'x' : '-',
                        '\0'
                    };
                    printf("0x%016lx-0x%016lx %8luKB %s   %s\n",
                           region.start, region.end, region.size / 1024,
                           perms, region.name.c_str());
                }
            } else {
                std::cerr << "[-] 获取内存映射失败: " << KernelMemoryInjector::result_to_string(result) << std::endl;
            }
        }
        
        // 读取内存
        if (read_memory && addr != 0) {
            if (size == 0) size = 64;
            std::cout << "[*] 读取内存 0x" << std::hex << addr << " (" << std::dec << size << " 字节)..." << std::endl;
            
            std::vector<uint8_t> buffer(size);
            result = injector.read_memory(addr, buffer.data(), size, force_mode);
            if (result == KernelInjectResult::SUCCESS) {
                std::cout << "[+] 读取成功:" << std::endl;
                std::cout << bytes_to_hex(buffer.data(), size) << std::endl;
            } else {
                std::cerr << "[-] 读取失败: " << KernelMemoryInjector::result_to_string(result) << std::endl;
            }
        }
        
        // 写入内存
        if (write_memory && addr != 0 && !hex_data.empty()) {
            auto bytes = hex_to_bytes(hex_data);
            std::cout << "[*] 写入内存 0x" << std::hex << addr << " (" << std::dec << bytes.size() << " 字节)..." << std::endl;
            
            result = injector.write_memory(addr, bytes.data(), bytes.size(), force_mode);
            if (result == KernelInjectResult::SUCCESS) {
                std::cout << "[+] 写入成功" << std::endl;
            } else {
                std::cerr << "[-] 写入失败: " << KernelMemoryInjector::result_to_string(result) << std::endl;
            }
        }
        
        // 注入 shellcode
        if (inject_shellcode && !file_path.empty()) {
            auto shellcode = read_file(file_path);
            if (shellcode.empty()) {
                std::cerr << "[-] 无法读取文件: " << file_path << std::endl;
            } else {
                std::cout << "[*] 注入 shellcode (" << shellcode.size() << " 字节)..." << std::endl;
                
                uint64_t injected_addr;
                result = injector.inject_shellcode(shellcode.data(), shellcode.size(), injected_addr);
                if (result == KernelInjectResult::SUCCESS) {
                    std::cout << "[+] Shellcode 注入成功，地址: 0x" << std::hex << injected_addr << std::endl;
                } else {
                    std::cerr << "[-] 注入失败: " << KernelMemoryInjector::result_to_string(result) << std::endl;
                }
            }
        }
        
        // 安装 hook
        if (install_hook && hook_target != 0 && !file_path.empty()) {
            auto hook_code = read_file(file_path);
            if (hook_code.empty()) {
                std::cerr << "[-] 无法读取 hook 代码文件: " << file_path << std::endl;
            } else {
                std::cout << "[*] 安装 hook 到 0x" << std::hex << hook_target << "..." << std::endl;
                
                KernelHookInfo hook_info;
                result = injector.install_hook(hook_target, hook_code.data(), hook_code.size(), hook_info);
                if (result == KernelInjectResult::SUCCESS) {
                    std::cout << "[+] Hook 安装成功!" << std::endl;
                    std::cout << "    目标地址: 0x" << std::hex << hook_info.target_addr << std::endl;
                    std::cout << "    Hook 地址: 0x" << std::hex << hook_info.hook_addr << std::endl;
                    std::cout << "    跳板地址: 0x" << std::hex << hook_info.trampoline_addr << std::endl;
                } else {
                    std::cerr << "[-] Hook 安装失败: " << KernelMemoryInjector::result_to_string(result) << std::endl;
                }
            }
        }
        
        // 显示进程信息
        std::string cmdline;
        if (injector.get_process_cmdline(cmdline) == KernelInjectResult::SUCCESS) {
            std::cout << "[*] 进程命令行: " << cmdline << std::endl;
        }
        
        uint64_t mem_size;
        if (injector.get_physical_memory_size(mem_size) == KernelInjectResult::SUCCESS) {
            std::cout << "[*] 物理内存占用: " << mem_size << " KB" << std::endl;
        }
    }
    
    std::cout << "[*] 操作完成" << std::endl;
    return 0;
}