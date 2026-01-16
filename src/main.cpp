/**
 * proc-mem-inject - 运行时内存注入工具
 * 
 * 特点：
 * - 不修改任何文件（绕过文件完整性检测）
 * - 不使用 ptrace（绕过 ptrace 检测）
 * - 纯内存操作，进程重启后痕迹消失
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
#include "memory_injector.h"

void print_usage(const char* prog) {
    printf("proc-mem-inject - 运行时内存注入 (不修改文件，不用ptrace)\n\n");
    printf("用法:\n");
    printf("  %s -k <root_key> -p <pid> [操作]\n\n", prog);
    printf("操作:\n");
    printf("  --maps                    显示内存映射\n");
    printf("  --read <addr> -s <size>   读取内存\n");
    printf("  --write <addr> -d <hex>   写入内存 (hex格式如: 1F2003D5)\n");
    printf("  --inject -c <file>        注入 shellcode 文件\n");
    printf("  --hook <addr> -c <file>   在指定地址安装 hook\n");
    printf("  --find <module>           查找模块基址\n\n");
    printf("示例:\n");
    printf("  # 查看内存映射\n");
    printf("  %s -k \"key\" -p 1234 --maps\n\n", prog);
    printf("  # 读取内存\n");
    printf("  %s -k \"key\" -p 1234 --read 0x7000000000 -s 64\n\n", prog);
    printf("  # 写入 NOP 指令\n");
    printf("  %s -k \"key\" -p 1234 --write 0x7000001000 -d 1F2003D5\n\n", prog);
    printf("  # 查找 libil2cpp.so 基址\n");
    printf("  %s -k \"key\" -p 1234 --find libil2cpp.so\n", prog);
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
    std::string root_key;
    pid_t pid = 0;
    std::string operation;
    uint64_t addr = 0;
    size_t size = 0;
    std::string hex_data;
    std::string file_path;
    std::string module_name;

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
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            hex_data = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            file_path = argv[++i];
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

    injector.detach();
    return 0;
}
