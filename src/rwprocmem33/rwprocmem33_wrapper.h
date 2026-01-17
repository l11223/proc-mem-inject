/**
 * rwProcMem33 用户态接口封装
 * 
 * 这个文件整合了 rwProcMem33 的所有用户态接口
 * 提供简洁的 C++ 封装供我们的项目使用
 */

#ifndef RWPROCMEM33_WRAPPER_H
#define RWPROCMEM33_WRAPPER_H

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <elf.h>
#include <random>
#include <algorithm>
#include <numeric>
#include <filesystem>

// 基础类型定义
typedef int BOOL;
#define TRUE 1
#define FALSE 0

// 内存保护标志
#define PAGE_NOACCESS 1
#define PAGE_READONLY 2
#define PAGE_READWRITE 4
#define PAGE_WRITECOPY 8
#define PAGE_EXECUTE 16
#define PAGE_EXECUTE_READ 32
#define PAGE_EXECUTE_READWRITE 64

// 内存类型
#define MEM_MAPPED 262144
#define MEM_PRIVATE 131072

// 驱动内存区域信息
#pragma pack(1)
typedef struct {
    uint64_t baseaddress;
    uint64_t size;
    uint32_t protection;
    uint32_t type;
    char name[4096];
} DRIVER_REGION_INFO;
#pragma pack()

namespace rwprocmem33 {

// 缓冲区池 (避免频繁分配内存)
class IoctlBufferPool {
    static constexpr size_t kDefaultBuffer = 4096;
    char _smallBuf[kDefaultBuffer];
    char* _largeBuf = nullptr;
    size_t _largeBufSize = 0;

public:
    ~IoctlBufferPool() {
        if (_largeBuf) {
            delete[] _largeBuf;
        }
    }

    char* getBuffer(size_t capacity) {
        if (capacity <= kDefaultBuffer) {
            return _smallBuf;
        }
        if (_largeBufSize < capacity) {
            if (_largeBuf) {
                delete[] _largeBuf;
            }
            _largeBuf = new (std::nothrow) char[capacity];
            _largeBufSize = _largeBuf ? capacity : 0;
        }
        return _largeBuf;
    }
};

// 驱动命令定义
enum {
    CMD_INIT_DEVICE_INFO = 1,
    CMD_OPEN_PROCESS,
    CMD_READ_PROCESS_MEMORY,
    CMD_WRITE_PROCESS_MEMORY,
    CMD_CLOSE_PROCESS,
    CMD_GET_PROCESS_MAPS_COUNT,
    CMD_GET_PROCESS_MAPS_LIST,
    CMD_CHECK_PROCESS_ADDR_PHY,
    CMD_GET_PID_LIST,
    CMD_SET_PROCESS_ROOT,
    CMD_GET_PROCESS_RSS,
    CMD_GET_PROCESS_CMDLINE_ADDR,
    CMD_HIDE_KERNEL_MODULE,
};

// ioctl 请求结构
#pragma pack(push, 1)
struct IoctlRequest {
    char cmd = 0;
    uint64_t param1 = 0;
    uint64_t param2 = 0;
    uint64_t param3 = 0;
    uint64_t bufSize = 0;
};

struct InitDeviceInfo {
    int pid = 0;
    int tid = 0;
    char myName[16 + 1] = {0};
    char myAuxv[1024] = {0};
    int myAuxvSize = 0;
};

struct MapEntry {
    uint64_t start = 0;
    uint64_t end = 0;
    unsigned char flags[4] = {0};
    char path[1024] = {0};
};

struct ArgInfo {
    uint64_t arg_start = 0;
    uint64_t arg_end = 0;
};
#pragma pack(pop)

/**
 * rwProcMem33 驱动接口封装类
 */
class RwProcMem33Driver {
public:
    RwProcMem33Driver() : m_fd(-1) {}
    
    ~RwProcMem33Driver() {
        disconnect();
    }

    // 连接驱动
    int connect(const std::string& auth_key) {
        namespace fs = std::filesystem;
        fs::path node_path = fs::path("/proc") / auth_key / auth_key;
        const char* path_cstr = node_path.c_str();
        
        // 修改权限
        if (chmod(path_cstr, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) != 0) {
            return -errno;
        }
        
        // 打开设备节点
        m_fd = open(path_cstr, O_RDWR);
        if (m_fd < 0) {
            return -errno;
        }
        
        // 初始化设备信息
        if (!init_device_info()) {
            close(m_fd);
            m_fd = -1;
            return -1;
        }
        
        return 0;
    }

    // 断开驱动
    void disconnect() {
        if (m_fd >= 0) {
            close(m_fd);
            m_fd = -1;
        }
    }

    // 检查连接状态
    bool is_connected() const {
        return m_fd >= 0;
    }

    // 隐藏内核模块
    bool hide_kernel_module() {
        if (m_fd < 0) return FALSE;
        return my_ioctl(CMD_HIDE_KERNEL_MODULE, 0, 0, 0, nullptr, 0) == 0;
    }

    // 打开进程
    uint64_t open_process(uint64_t pid) {
        if (m_fd < 0 || pid == 0) return 0;
        
        uint64_t handle = 0;
        ssize_t res = my_ioctl(CMD_OPEN_PROCESS, pid, 0, 0, (char*)&handle, sizeof(handle));
        return (res == 0) ? handle : 0;
    }

    // 关闭进程
    bool close_process(uint64_t handle) {
        if (m_fd < 0 || !handle) return FALSE;
        return my_ioctl(CMD_CLOSE_PROCESS, handle, 0, 0, nullptr, 0) == 0;
    }

    // 读取进程内存
    bool read_process_memory(uint64_t handle, uint64_t addr, void* buf, size_t size, size_t* bytes_read, bool force_read) {
        if (addr == 0 || m_fd < 0 || handle == 0 || size == 0) {
            return FALSE;
        }
        
        ssize_t out_read = my_ioctl(CMD_READ_PROCESS_MEMORY, handle, addr, force_read ? 1 : 0, (char*)buf, size);
        if (out_read <= 0) {
            return FALSE;
        }
        
        if (bytes_read) {
            *bytes_read = out_read;
        }
        return TRUE;
    }

    // 写入进程内存
    bool write_process_memory(uint64_t handle, uint64_t addr, const void* buf, size_t size, size_t* bytes_written, bool force_write) {
        if (addr == 0 || m_fd < 0 || !handle || size == 0) {
            return FALSE;
        }
        
        ssize_t out_write = my_ioctl(CMD_WRITE_PROCESS_MEMORY, handle, addr, force_write ? 1 : 0, (char*)buf, size);
        if (out_write <= 0) {
            return FALSE;
        }
        
        if (bytes_written) {
            *bytes_written = out_write;
        }
        return TRUE;
    }

    // 获取内存映射列表
    bool get_memory_maps(uint64_t handle, bool physical_only, std::vector<DRIVER_REGION_INFO>& regions) {
        if (m_fd < 0 || !handle) return FALSE;
        
        // 获取内存区域数量
        ssize_t count = my_ioctl(CMD_GET_PROCESS_MAPS_COUNT, handle, 0, 0, nullptr, 0);
        if (count <= 0) {
            return FALSE;
        }
        
        // 分配缓冲区
        static thread_local IoctlBufferPool pool;
        uint64_t buf_len = sizeof(MapEntry) * (count + 50);
        char* buf = pool.getBuffer(buf_len);
        if (!buf) return FALSE;
        
        memset(buf, 0, buf_len);
        
        // 获取内存映射列表
        ssize_t res = my_ioctl(CMD_GET_PROCESS_MAPS_LIST, handle, 0, 0, buf, buf_len);
        if (res <= 0) {
            return FALSE;
        }
        
        // 解析结果
        auto entries = reinterpret_cast<MapEntry*>(buf);
        for (ssize_t i = 0; i < res; ++i) {
            const MapEntry& e = entries[i];
            DRIVER_REGION_INFO info = {0};
            info.baseaddress = e.start;
            info.size = e.end - e.start;
            
            bool r = e.flags[0], w = e.flags[1], x = e.flags[2];
            if (x) {
                info.protection = w ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
            } else {
                if (w) info.protection = PAGE_READWRITE;
                else if (r) info.protection = PAGE_READONLY;
                else info.protection = PAGE_NOACCESS;
            }
            
            info.type = (e.flags[3] ? MEM_MAPPED : MEM_PRIVATE);
            strncpy(info.name, e.path, sizeof(info.name) - 1);
            
            if (physical_only) {
                // 只返回有物理内存的区域
                DRIVER_REGION_INFO cur = info;
                bool in_phy = false;
                for (uint64_t addr = e.start; addr < e.end; addr += getpagesize()) {
                    if (check_memory_valid(handle, addr)) {
                        if (!in_phy) {
                            in_phy = true;
                            cur.baseaddress = addr;
                        }
                    } else if (in_phy) {
                        in_phy = false;
                        cur.size = addr - cur.baseaddress;
                        regions.push_back(cur);
                    }
                }
                if (in_phy) {
                    cur.size = e.end - cur.baseaddress;
                    regions.push_back(cur);
                }
            } else {
                regions.push_back(info);
            }
        }
        
        return TRUE;
    }

    // 检查内存地址有效性
    bool check_memory_valid(uint64_t handle, uint64_t addr) {
        if (m_fd < 0 || !handle) return FALSE;
        return my_ioctl(CMD_CHECK_PROCESS_ADDR_PHY, handle, addr, 0, nullptr, 0) == 1;
    }

    // 提升进程权限到 root
    bool set_process_root(uint64_t handle) {
        if (m_fd < 0 || !handle) return FALSE;
        return my_ioctl(CMD_SET_PROCESS_ROOT, handle, 0, 0, nullptr, 0) == 0;
    }

    // 获取进程物理内存大小
    bool get_physical_memory_size(uint64_t handle, uint64_t& size_kb) {
        if (m_fd < 0 || !handle) return FALSE;
        
        uint64_t out = 0;
        ssize_t res = my_ioctl(CMD_GET_PROCESS_RSS, handle, 0, 0, (char*)&out, sizeof(out));
        if (res != 0) {
            return FALSE;
        }
        
        size_kb = out;
        return TRUE;
    }

    // 获取进程命令行
    bool get_process_cmdline(uint64_t handle, std::string& cmdline) {
        if (m_fd < 0 || !handle) return FALSE;
        
        ArgInfo arg_info = {0};
        ssize_t res = my_ioctl(CMD_GET_PROCESS_CMDLINE_ADDR, handle, 0, 0, (char*)&arg_info, sizeof(arg_info));
        if (res != 0 || arg_info.arg_start == 0) {
            return FALSE;
        }
        
        uint64_t len = arg_info.arg_end - arg_info.arg_start;
        if (len == 0 || len > 4096) {
            return FALSE;
        }
        
        std::vector<char> buf(len + 1, 0);
        size_t bytes_read = 0;
        if (!read_process_memory(handle, arg_info.arg_start, buf.data(), len, &bytes_read, false)) {
            return FALSE;
        }
        
        cmdline = buf.data();
        return TRUE;
    }

    // 获取 PID 列表
    bool get_pid_list(std::vector<int>& pids) {
        if (m_fd < 0) return FALSE;
        
        ssize_t count = my_ioctl(CMD_GET_PID_LIST, 0, 0, 0, nullptr, 0);
        if (count <= 0) {
            return FALSE;
        }
        
        static thread_local IoctlBufferPool pool;
        uint64_t len = (uint64_t)count * sizeof(int);
        char* buf = pool.getBuffer(len);
        if (!buf) {
            return FALSE;
        }
        
        memset(buf, 0, len);
        ssize_t count2 = my_ioctl(CMD_GET_PID_LIST, 0, 0, 0, buf, len);
        if (count2 != count) {
            return FALSE;
        }
        
        for (int i = 0; i < count; i++) {
            int pid = *(int*)(buf + i * sizeof(int));
            pids.push_back(pid);
        }
        
        return TRUE;
    }

private:
    // 执行 ioctl 操作
    ssize_t my_ioctl(char cmd, uint64_t param1, uint64_t param2, uint64_t param3, char* buf, uint64_t buf_size) {
        constexpr size_t header_size = sizeof(IoctlRequest);
        size_t total_size = header_size + buf_size;
        
        static thread_local IoctlBufferPool pool;
        char* io_buf = pool.getBuffer(total_size);
        if (!io_buf) return -ENOMEM;
        
        IoctlRequest* req = reinterpret_cast<IoctlRequest*>(io_buf);
        req->cmd = cmd;
        req->param1 = param1;
        req->param2 = param2;
        req->param3 = param3;
        req->bufSize = buf_size;
        
        if (buf_size > 0 && buf) {
            std::memcpy(io_buf + header_size, buf, buf_size);
        }
        
        auto out_read = ::read(m_fd, io_buf, total_size);
        
        if (buf_size > 0 && buf) {
            std::memcpy(buf, io_buf + header_size, buf_size);
        }
        
        return out_read;
    }

    // 初始化设备信息
    bool init_device_info() {
        InitDeviceInfo dev_info = {0};
        dev_info.pid = getpid();
        dev_info.tid = gettid();
        
        // 获取 auxv 签名
        std::vector<unsigned long> auxv = get_auxv_signature();
        int auxv_byte_count = auxv.size() * sizeof(unsigned long);
        if (auxv_byte_count == 0 || auxv_byte_count >= sizeof(dev_info.myAuxv)) {
            return FALSE;
        }
        
        memcpy(&dev_info.myAuxv, reinterpret_cast<const uint8_t*>(auxv.data()), auxv_byte_count);
        dev_info.myAuxvSize = auxv_byte_count;
        
        // 生成随机进程名
        char old_name[17] = {0};
        if (prctl(PR_GET_NAME, old_name, 0, 0, 0)) {
            return FALSE;
        }
        
        std::vector<uint8_t> random_name = generate_unique_non_zero_bytes(15);
        random_name.push_back(0);
        
        if (prctl(PR_SET_NAME, random_name.data(), 0, 0, 0)) {
            return FALSE;
        }
        
        strncpy(dev_info.myName, (char*)random_name.data(), sizeof(dev_info.myName) - 1);
        
        ssize_t ret = my_ioctl(CMD_INIT_DEVICE_INFO, 0, 0, 0, (char*)&dev_info, sizeof(dev_info));
        
        // 恢复原进程名
        prctl(PR_SET_NAME, old_name, 0, 0, 0);
        
        return (ret == 0);
    }

    // 获取 auxv 签名
    std::vector<unsigned long> get_auxv_signature() {
        std::vector<unsigned long> sig;
        int fd = open("/proc/self/auxv", O_RDONLY);
        if (fd < 0) {
            return sig;
        }
        
        while (true) {
            unsigned long a_type, a_val;
            ssize_t nr;
            
            nr = read(fd, &a_type, sizeof(a_type));
            if (nr != sizeof(a_type)) break;
            
            nr = read(fd, &a_val, sizeof(a_val));
            if (nr != sizeof(a_val)) break;
            
            sig.push_back(a_type);
            sig.push_back(a_val);
            
            if (a_type == AT_NULL) break;
        }
        
        close(fd);
        return sig;
    }

    // 生成唯一的非零字节序列
    std::vector<uint8_t> generate_unique_non_zero_bytes(std::size_t count) {
        if (count == 0 || count > 255) {
            return {};
        }
        
        std::vector<uint8_t> pool(255);
        std::iota(pool.begin(), pool.end(), 1);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(pool.begin(), pool.end(), gen);
        
        return std::vector<uint8_t>(pool.begin(), pool.begin() + count);
    }

private:
    int m_fd;
};

} // namespace rwprocmem33

#endif // RWPROCMEM33_WRAPPER_H