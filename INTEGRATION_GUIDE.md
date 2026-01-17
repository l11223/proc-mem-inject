# 🚀 rwProcMem33 集成指南

## 📋 概述

本指南详细说明如何将 rwProcMem33 驱动完全集成到我们的项目中，实现真正的内核级无痕注入。

## 🎯 当前状态

### ✅ 已完成
1. **项目结构搭建**
   - 下载了 rwProcMem33 源码到 `deps/rwProcMem33/`
   - 创建了 `KernelMemoryInjector` 类框架
   - 设计了完整的 API 接口
   - 更新了 CMakeLists.txt 支持双版本编译

2. **架构设计**
   - 传统版本：`stealth_mem` (基于 /proc/pid/mem)
   - 内核版本：`kernel_mem` (基于 rwProcMem33 驱动)
   - 清晰的接口分离和适配层设计

### 🔄 待完成
1. **rwProcMem33 用户态库集成**
2. **驱动编译和部署**
3. **接口适配层实现**
4. **测试和验证**

---

## 🔧 集成步骤

### 步骤 1: 提取 rwProcMem33 用户态接口

我们需要从 rwProcMem33 项目中提取以下关键文件：

```bash
# 核心用户态接口文件
deps/rwProcMem33/rwProcMem33Module/testKo/jni/
├── MemoryReaderWriter39.h      # 主要接口类
├── IMemReaderWriterProxy.h     # 接口定义
├── IoctlBufferPool.h          # 缓冲区管理
└── testKo.cpp                 # 使用示例
```

**操作**:
1. 复制这些文件到我们的 `src/rwprocmem33/` 目录
2. 修改头文件包含路径
3. 适配我们的项目结构

### 步骤 2: 实现 RwProcMem33Adapter 适配层

当前 `kernel_memory_injector.cpp` 中的 `RwProcMem33Adapter` 类是占位符，需要实现：

```cpp
class RwProcMem33Adapter {
private:
    CMemoryReaderWriter m_driver;  // rwProcMem33 的驱动接口
    
public:
    int connect_driver(const std::string& auth_key) {
        return m_driver.ConnectDriver(auth_key.c_str());
    }
    
    bool read_process_memory(uint64_t handle, uint64_t addr, void* buf, size_t size, size_t* bytes_read, bool force_read) {
        return m_driver.ReadProcessMemory(handle, addr, buf, size, bytes_read, force_read);
    }
    
    // ... 其他接口实现
};
```

### 步骤 3: 驱动编译和部署

rwProcMem33 需要编译内核驱动模块：

```bash
# 进入驱动源码目录
cd deps/rwProcMem33/rwProcMem33Module/rwProcMem_module/

# 编译驱动 (需要对应内核源码)
make

# 生成的驱动文件
rwProcMem_module.ko
```

**注意事项**:
1. 需要目标设备的内核源码或头文件
2. 可能需要修改 5-6 处地方适配不同机型
3. 需要 root 权限加载驱动

### 步骤 4: 驱动加载脚本

创建自动化的驱动加载脚本：

```bash
#!/system/bin/sh
# load_driver.sh

DRIVER_PATH="/data/local/tmp/rwProcMem_module.ko"
AUTH_KEY="e84523d7b60d5d341a7c4d1861773ecd"

# 检查 root 权限
if [ "$(id -u)" != "0" ]; then
    echo "需要 root 权限"
    exit 1
fi

# 加载驱动
insmod $DRIVER_PATH

# 创建隐蔽通信节点
mkdir -p /proc/$AUTH_KEY
chmod 666 /proc/$AUTH_KEY/$AUTH_KEY

echo "驱动加载完成"
```

---

## 🛠️ 详细实现计划

### 阶段 1: 用户态库集成 (1-2 天)

**目标**: 让 `kernel_mem` 能够编译通过

**任务**:
1. 复制 rwProcMem33 用户态文件到项目
2. 修改 `RwProcMem33Adapter` 类，调用真实的 rwProcMem33 接口
3. 解决编译依赖和链接问题
4. 更新 CMakeLists.txt

**验证**: `kernel_mem` 编译成功，能够尝试连接驱动

### 阶段 2: 驱动编译适配 (2-3 天)

**目标**: 成功编译出适配目标设备的驱动

**任务**:
1. 获取目标设备的内核信息
2. 适配 rwProcMem33 驱动源码
3. 解决编译错误和警告
4. 测试驱动加载

**验证**: 驱动能够在目标设备上成功加载

### 阶段 3: 功能测试验证 (1-2 天)

**目标**: 验证所有核心功能正常工作

**任务**:
1. 测试驱动连接和隐藏
2. 测试进程附加和权限提升
3. 测试内存读写操作
4. 测试 hook 安装和移除
5. 对比传统版本和内核版本的效果

**验证**: 所有功能正常，无痕效果明显

---

## 📁 最终项目结构

```
proc-mem-inject/
├── src/
│   ├── main.cpp                    # 传统版本主程序
│   ├── memory_injector.cpp         # 传统版本实现
│   ├── kernel_main.cpp             # 内核版本主程序
│   ├── kernel_memory_injector.cpp  # 内核版本实现
│   ├── kernel_memory_injector.h    # 内核版本头文件
│   └── rwprocmem33/               # rwProcMem33 用户态库
│       ├── MemoryReaderWriter39.h
│       ├── IMemReaderWriterProxy.h
│       └── IoctlBufferPool.h
├── deps/
│   ├── kernel_root_kit/           # SKRoot Lite 依赖
│   └── rwProcMem33/              # rwProcMem33 完整源码
├── scripts/
│   ├── load_driver.sh            # 驱动加载脚本
│   └── build_driver.sh           # 驱动编译脚本
├── outputs/
│   └── arm64-v8a/
│       ├── stealth_mem           # 传统版本可执行文件
│       ├── kernel_mem            # 内核版本可执行文件
│       └── rwProcMem_module.ko   # 内核驱动模块
└── docs/
    ├── UPGRADE_PLAN.md           # 升级计划
    ├── INTEGRATION_GUIDE.md      # 集成指南 (本文件)
    └── rwProcMem33_integration.md # rwProcMem33 详细分析
```

---

## 🎯 使用方式对比

### 传统版本 (stealth_mem)
```bash
# 基于 /proc/pid/mem，需要 SKRoot Lite
adb push stealth_mem /data/local/tmp/
adb shell /data/local/tmp/stealth_mem -k "GD5IyCe0opOqirn6Qs1qDNVFWqpmYc3cNAd9pOgJ8erzOpMf" -p 1234 --read -a 0x7000000000 -s 64
```

### 内核版本 (kernel_mem)
```bash
# 基于 rwProcMem33 驱动，真正的内核级操作
adb push kernel_mem /data/local/tmp/
adb push rwProcMem_module.ko /data/local/tmp/
adb shell su -c "insmod /data/local/tmp/rwProcMem_module.ko"
adb shell /data/local/tmp/kernel_mem --hide -p 1234 --read -a 0x7000000000 -s 64
```

---

## 🔍 技术优势对比

| 特性 | 传统版本 (stealth_mem) | 内核版本 (kernel_mem) |
|------|----------------------|---------------------|
| **操作层级** | 用户态 | 内核态 |
| **检测难度** | 中等 | 极难 |
| **权限要求** | Root + /proc 访问 | 内核驱动权限 |
| **痕迹残留** | 文件描述符、进程痕迹 | 几乎无痕迹 |
| **兼容性** | 依赖 /proc 文件系统 | 硬件级支持 |
| **反检测** | 基础反检测 | 驱动级隐藏 |
| **调试能力** | 软件断点 | 硬件断点 |
| **Android 15** | 可能受限 | 完美支持 |

---

## 🚨 注意事项

### 1. 法律和道德
- 仅供学习研究使用
- 不得用于非法用途
- 遵守当地法律法规

### 2. 技术风险
- 内核驱动操作有系统风险
- 错误的驱动可能导致系统崩溃
- 建议在测试设备上进行

### 3. 兼容性
- 不同内核版本可能需要适配
- 不同厂商的内核可能有差异
- Android 版本升级可能影响兼容性

---

## 📞 下一步行动

1. **立即开始**: 阶段 1 用户态库集成
2. **重点关注**: 驱动编译适配 (最关键)
3. **逐步验证**: 每个阶段完成后独立测试
4. **持续优化**: 根据测试结果调整方案

虎虎大王，这个集成方案怎么样？我们可以先从阶段 1 开始，把用户态库集成进来！🚀