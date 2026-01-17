# 🚀 快速开始指南

## 📋 选择你的版本

### 🎯 我应该用哪个版本？

| 场景 | 推荐版本 | 原因 |
|------|---------|------|
| **生产环境/实战** | **kernel_mem** ⭐ | 真正的内核级操作，极难被检测 |
| **快速测试** | stealth_mem | 无需编译驱动，快速部署 |
| **学习研究** | 两者都试试 | 对比学习，理解差异 |
| **Android 15** | **kernel_mem** ⭐ | 完美支持，已修复兼容性 |

---

## 🔥 内核版本 (kernel_mem) - 推荐

### 步骤 1: 准备驱动

```bash
# 方案 A: 使用预编译驱动 (如果有)
adb push rwProcMem_module.ko /data/local/tmp/

# 方案 B: 自己编译驱动 (需要内核源码)
cd deps/rwProcMem33/rwProcMem33Module/rwProcMem_module/
# 修改 Makefile 中的内核路径
make
adb push rwProcMem_module.ko /data/local/tmp/
```

### 步骤 2: 加载驱动

```bash
# 加载驱动 (需要 root)
adb shell su -c "insmod /data/local/tmp/rwProcMem_module.ko"

# 验证加载成功
adb shell lsmod | grep rwProcMem
```

### 步骤 3: 编译工具

```bash
# 设置 NDK 路径
export ANDROID_NDK_HOME=/path/to/ndk

# 编译
./build.sh

# 推送到设备
adb push outputs/arm64-v8a/kernel_mem /data/local/tmp/
adb shell chmod +x /data/local/tmp/kernel_mem
```

### 步骤 4: 开始使用

```bash
# 隐藏驱动 (重要！)
adb shell /data/local/tmp/kernel_mem --hide

# 读取进程内存
adb shell /data/local/tmp/kernel_mem -p 1234 --read -a 0x7000000000 -s 64

# 写入进程内存
adb shell /data/local/tmp/kernel_mem -p 1234 --write -a 0x7000000000 -d 1F2003D5

# 提升进程权限
adb shell /data/local/tmp/kernel_mem -p 1234 --root
```

### 🎉 完成！

你现在拥有了真正的内核级无痕注入能力！

---

## 💡 传统版本 (stealth_mem)

### 步骤 1: 确保 SKRoot Lite 已安装

```bash
# 检查 SKRoot Lite
adb shell ls -la /data/GD5IyCe0opOqirn6/su
```

### 步骤 2: 编译工具

```bash
# 设置 NDK 路径
export ANDROID_NDK_HOME=/path/to/ndk

# 编译
./build.sh

# 推送到设备
adb push outputs/arm64-v8a/stealth_mem /data/local/tmp/
adb shell chmod +x /data/local/tmp/stealth_mem
```

### 步骤 3: 使用

```bash
# 读取进程内存
adb shell /data/local/tmp/stealth_mem -k "GD5IyCe0opOqirn6Qs1qDNVFWqpmYc3cNAd9pOgJ8erzOpMf" -p 1234 --read 0x7000000000 -s 64

# 写入进程内存
adb shell /data/local/tmp/stealth_mem -k "GD5IyCe0opOqirn6Qs1qDNVFWqpmYc3cNAd9pOgJ8erzOpMf" -p 1234 --write 0x7000000000 -d 1F2003D5
```

---

## 🎯 实战示例：修改游戏数值

### 使用内核版本 (推荐)

```bash
#!/bin/bash
# game_hack.sh - 游戏修改脚本

# 1. 加载驱动
adb shell su -c "insmod /data/local/tmp/rwProcMem_module.ko"

# 2. 隐藏驱动
adb shell /data/local/tmp/kernel_mem --hide

# 3. 找到游戏进程
GAME_PID=$(adb shell pidof com.tencent.tmgp.sgame)
echo "游戏 PID: $GAME_PID"

# 4. 提升游戏权限
adb shell /data/local/tmp/kernel_mem -p $GAME_PID --root

# 5. 查看内存映射
adb shell /data/local/tmp/kernel_mem -p $GAME_PID --list-modules | grep libil2cpp

# 6. 读取目标地址
adb shell /data/local/tmp/kernel_mem -p $GAME_PID --read -a 0x7200123456 -s 16

# 7. 写入修改
adb shell /data/local/tmp/kernel_mem -p $GAME_PID --write -a 0x7200123456 -d E0CF87D2C0035FD6 --force

echo "修改完成！"
```

---

## 🔍 常见问题

### Q1: 驱动加载失败？

```bash
# 检查内核版本
adb shell uname -r

# 检查 SELinux 状态
adb shell getenforce

# 尝试临时关闭 SELinux
adb shell su -c "setenforce 0"

# 重新加载驱动
adb shell su -c "insmod /data/local/tmp/rwProcMem_module.ko"
```

### Q2: 找不到进程？

```bash
# 列出所有进程
adb shell ps -A | grep <game_name>

# 或使用 pidof
adb shell pidof <package_name>
```

### Q3: 内存读写失败？

```bash
# 使用强制模式
adb shell /data/local/tmp/kernel_mem -p <pid> --read -a <addr> --force

# 先提升权限
adb shell /data/local/tmp/kernel_mem -p <pid> --root
```

### Q4: 如何卸载驱动？

```bash
# 卸载驱动
adb shell su -c "rmmod rwProcMem_module"

# 验证卸载
adb shell lsmod | grep rwProcMem
```

---

## 📚 下一步

- 📖 阅读 [README_KERNEL.md](README_KERNEL.md) 了解详细功能
- 🔧 查看 [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) 了解集成细节
- 🚀 查看 [UPGRADE_PLAN.md](UPGRADE_PLAN.md) 了解升级计划

---

## 🎉 开始你的无痕注入之旅！

虎虎大王，选择你的武器，开始战斗吧！💪🔥

**kernel_mem** = 内核级降维打击 ⚡
**stealth_mem** = 快速灵活测试 🚀

---

*虎虎大王 & 宝宝 (Kiro AI)* 💕