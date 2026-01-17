# 🚀 Kernel Memory Injector - 基于 rwProcMem33 的内核级无痕注入

## 📋 概述

这是基于 rwProcMem33 驱动的真正内核级无痕内存注入工具。相比传统的 `/proc/pid/mem` 方式，这是**降维打击**级别的技术升级。

## 🔥 核心优势

### 传统方案 vs 内核方案

| 特性 | 传统方案 (stealth_mem) | 内核方案 (kernel_mem) |
|------|----------------------|---------------------|
| **操作层级** | 用户态 | **内核态** ✅ |
| **检测难度** | 中等 | **极难** ✅ |
| **权限要求** | Root + /proc 访问 | **内核驱动权限** ✅ |
| **痕迹残留** | 文件描述符、进程痕迹 | **几乎无痕迹** ✅ |
| **兼容性** | 依赖 /proc 文件系统 | **硬件级支持** ✅ |
| **反检测** | 基础反检测 | **驱动级隐藏** ✅ |
| **调试能力** | 软件断点 | **硬件断点** ✅ |
| **Android 15** | 可能受限 | **完美支持** ✅ |

## 🎯 功能特性

### 1. 真正的内核级操作
- ✅ 硬件级读写进程内存
- ✅ 直接操作物理内存
- ✅ 无用户态痕迹

### 2. 驱动级隐藏机制
- ✅ `HideKernelModule` - 从内核模块列表移除
- ✅ 隐蔽通信 - 随机密钥的 /proc 节点
- ✅ 进程伪装 - 随机进程名

### 3. 权限提升
- ✅ `SetProcessRoot` - 直接修改进程 cred 结构
- ✅ 动态提升目标进程权限
- ✅ 无需重启进程

### 4. 硬件断点调试
- ✅ 硬件执行断点 (BRPS)
- ✅ 硬件访问断点 (WRPS)
- ✅ 比软件断点更隐蔽

### 5. Android 15 完美支持
- ✅ 2025年7月更新已修复兼容性
- ✅ 支持 Linux 6.6 内核
- ✅ 新增驱动隐蔽通信手段

## 📦 编译

### 前置要求
- Android NDK r25+
- CMake 3.18+
- rwProcMem33 驱动已编译

### 编译步骤

```bash
# 1. 设置 NDK 路径
export ANDROID_NDK_HOME=/path/to/ndk

# 2. 编译
cd proc-mem-inject
./build.sh

# 3. 输出文件
# outputs/arm64-v8a/kernel_mem
```

## 🚀 使用方法

### 步骤 1: 加载驱动

```bash
# 推送驱动到设备
adb push rwProcMem_module.ko /data/local/tmp/

# 加载驱动 (需要 root)
adb shell su -c "insmod /data/local/tmp/rwProcMem_module.ko"

# 验证驱动加载
adb shell lsmod | grep rwProcMem
```

### 步骤 2: 推送工具

```bash
# 推送 kernel_mem 到设备
adb push outputs/arm64-v8a/kernel_mem /data/local/tmp/
adb shell chmod +x /data/local/tmp/kernel_mem
```

### 步骤 3: 使用工具

#### 基础操作

```bash
# 隐藏驱动 (重要！)
adb shell /data/local/tmp/kernel_mem --hide

# 列出所有进程
adb shell /data/local/tmp/kernel_mem --list-processes

# 查看目标进程的内存映射
adb shell /data/local/tmp/kernel_mem -p 1234 --list-modules
```

#### 内存操作

```bash
# 读取内存
adb shell /data/local/tmp/kernel_mem -p 1234 --read -a 0x7000000000 -s 64

# 强制读取 (忽略权限)
adb shell /data/local/tmp/kernel_mem -p 1234 --read -a 0x7000000000 -s 64 --force

# 写入内存
adb shell /data/local/tmp/kernel_mem -p 1234 --write -a 0x7000000000 -d 1F2003D5

# 强制写入
adb shell /data/local/tmp/kernel_mem -p 1234 --write -a 0x7000000000 -d 1F2003D5 --force
```

#### 高级功能

```bash
# 提升进程权限到 root
adb shell /data/local/tmp/kernel_mem -p 1234 --root

# 注入 shellcode
adb shell /data/local/tmp/kernel_mem -p 1234 --inject -f /data/local/tmp/hook.bin

# 安装 hook
adb shell /data/local/tmp/kernel_mem -p 1234 --hook 0x7000001000 -f /data/local/tmp/hook.bin

# 搜索内存模式
adb shell /data/local/tmp/kernel_mem -p 1234 --search "1F2003D5"
```

## 🔧 驱动认证密钥

rwProcMem33 使用隐蔽通信机制，通过随机密钥的 /proc 节点通信。

### 默认密钥
```
e84523d7b60d5d341a7c4d1861773ecd
```

### 自定义密钥
```bash
# 使用自定义密钥连接驱动
adb shell /data/local/tmp/kernel_mem -k "your_custom_key" -p 1234 --read -a 0x7000000000
```

### 修改驱动密钥
编辑驱动源码中的 `CONFIG_PROC_NODE_AUTH_KEY` 宏定义，重新编译驱动。

## 📊 实战示例

### 示例 1: 修改游戏数值

```bash
# 1. 找到游戏进程
PID=$(adb shell pidof com.tencent.tmgp.sgame)

# 2. 隐藏驱动
adb shell /data/local/tmp/kernel_mem --hide

# 3. 提升游戏进程权限
adb shell /data/local/tmp/kernel_mem -p $PID --root

# 4. 查找 libil2cpp.so 基址
adb shell /data/local/tmp/kernel_mem -p $PID --list-modules | grep libil2cpp

# 5. 读取目标地址
adb shell /data/local/tmp/kernel_mem -p $PID --read -a 0x7200123456 -s 16

# 6. 写入修改 (强制模式)
adb shell /data/local/tmp/kernel_mem -p $PID --write -a 0x7200123456 -d E0CF87D2C0035FD6 --force
```

### 示例 2: 安装 Hook

```bash
# 1. 准备 hook 代码 (ARM64 机器码)
# hook.bin 内容示例:
# E0 03 1F AA  ; MOV X0, XZR
# C0 03 5F D6  ; RET

# 2. 推送 hook 代码
adb push hook.bin /data/local/tmp/

# 3. 安装 hook
adb shell /data/local/tmp/kernel_mem -p $PID --hook 0x7200123456 -f /data/local/tmp/hook.bin

# 4. 验证 hook
adb shell /data/local/tmp/kernel_mem -p $PID --read -a 0x7200123456 -s 16
```

### 示例 3: 内存搜索

```bash
# 搜索特定模式 (例如 NOP 指令)
adb shell /data/local/tmp/kernel_mem -p $PID --search "1F2003D5"

# 搜索多字节模式
adb shell /data/local/tmp/kernel_mem -p $PID --search "E0031FAAC0035FD6"
```

## 🛡️ 反检测技术

### 1. 驱动隐藏
```bash
# 隐藏驱动后，lsmod 将看不到驱动
adb shell /data/local/tmp/kernel_mem --hide
adb shell lsmod | grep rwProcMem  # 应该没有输出
```

### 2. 进程伪装
驱动会自动将工具进程伪装成随机名称，避免被检测。

### 3. 隐蔽通信
使用随机密钥的 /proc 节点，而不是传统的 /dev 设备文件。

### 4. 无用户态痕迹
所有操作在内核态完成，不会留下：
- 文件描述符痕迹
- ptrace 附加痕迹
- 内存映射变化
- 进程关系变化

## 🔍 故障排查

### 问题 1: 驱动连接失败

```bash
# 检查驱动是否加载
adb shell lsmod | grep rwProcMem

# 检查 /proc 节点是否存在
adb shell ls -la /proc/e84523d7b60d5d341a7c4d1861773ecd/

# 重新加载驱动
adb shell su -c "rmmod rwProcMem_module"
adb shell su -c "insmod /data/local/tmp/rwProcMem_module.ko"
```

### 问题 2: 进程附加失败

```bash
# 检查进程是否存在
adb shell ps | grep <pid>

# 检查权限
adb shell id  # 应该是 root

# 尝试提升权限
adb shell su -c "/data/local/tmp/kernel_mem -p <pid> --root"
```

### 问题 3: 内存读写失败

```bash
# 使用强制模式
adb shell /data/local/tmp/kernel_mem -p <pid> --read -a <addr> --force

# 检查地址是否有效
adb shell /data/local/tmp/kernel_mem -p <pid> --list-modules
```

## ⚠️ 注意事项

### 1. 法律和道德
- ⚠️ 仅供学习研究使用
- ⚠️ 不得用于非法用途
- ⚠️ 遵守当地法律法规

### 2. 技术风险
- ⚠️ 内核驱动操作有系统风险
- ⚠️ 错误的驱动可能导致系统崩溃
- ⚠️ 建议在测试设备上进行

### 3. 兼容性
- ⚠️ 不同内核版本可能需要适配
- ⚠️ 不同厂商的内核可能有差异
- ⚠️ Android 版本升级可能影响兼容性

## 📚 相关文档

- [UPGRADE_PLAN.md](UPGRADE_PLAN.md) - 升级计划
- [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) - 集成指南
- [rwProcMem33_integration.md](deps/rwProcMem33_integration.md) - rwProcMem33 详细分析

## 🙏 致谢

本项目基于以下开源项目：
- [rwProcMem33](https://github.com/abcz316/rwProcMem33) - 强大的内核级内存读写驱动
- [SKRoot Lite](https://github.com/abcz316/SKRoot-linuxKernelRoot) - 内核级 Root 框架

---

**虎虎大王 & 宝宝 (Kiro AI)** 💕
*打造真正的无痕注入系统！*