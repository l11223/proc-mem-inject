# proc-mem-inject - 运行时内存注入工具

**真正的无痕注入！**

## 特点

- ❌ **不修改任何文件** → 绕过文件完整性检测
- ❌ **不使用 ptrace** → 绕过 ptrace 检测  
- ✅ **纯内存操作** → 通过 /proc/pid/mem 直接读写
- ✅ **运行时注入** → 游戏已启动也能注入
- ✅ **痕迹自动消失** → 进程重启后无痕迹

## 原理

```
1. SKRoot get_root() 获取 ROOT 权限
2. 打开 /proc/<pid>/mem 直接读写目标进程内存
3. 在目标进程内存中找空闲区域（代码空洞）
4. 写入 hook 代码
5. 修改目标函数入口跳转到 hook
```

## 前置条件

1. 设备已刷入 SKRoot Lite 补丁的内核
2. 知道 SKRoot 的 ROOT 密钥
3. 目标进程是 64 位 (ARM64)

## 编译

### Linux/macOS

```bash
export ANDROID_NDK_HOME=/path/to/ndk
chmod +x build.sh
./build.sh
```

### Windows

```batch
set ANDROID_NDK_HOME=C:\path\to\ndk
build.bat
```

## 使用方法

### 1. 推送到设备

```bash
adb push outputs/arm64-v8a/stealth_mem /data/local/tmp/
adb shell chmod +x /data/local/tmp/stealth_mem
```

### 2. 查看目标进程内存映射

```bash
# 先获取目标进程 PID
adb shell pidof com.tencent.tmgp.sgame

# 查看内存映射
adb shell /data/local/tmp/stealth_mem -k "your_root_key" -p 1234 --maps
```

### 3. 查找模块基址

```bash
# 查找 libil2cpp.so 基址
adb shell /data/local/tmp/stealth_mem -k "key" -p 1234 --find libil2cpp.so

# 查找 libunity.so 基址
adb shell /data/local/tmp/stealth_mem -k "key" -p 1234 --find libunity.so
```

### 4. 读取内存

```bash
# 读取 64 字节
adb shell /data/local/tmp/stealth_mem -k "key" -p 1234 --read 0x7000000000 -s 64
```

### 5. 写入内存

```bash
# 写入 NOP 指令 (ARM64: 1F2003D5)
adb shell /data/local/tmp/stealth_mem -k "key" -p 1234 --write 0x7000001000 -d 1F2003D5

# 写入多条 NOP
adb shell /data/local/tmp/stealth_mem -k "key" -p 1234 --write 0x7000001000 -d 1F2003D51F2003D51F2003D51F2003D5
```

### 6. 安装 Hook

```bash
# 准备 hook 代码文件 (ARM64 机器码)
adb shell /data/local/tmp/stealth_mem -k "key" -p 1234 --hook 0x7000001000 -c /data/local/tmp/hook.bin
```

## 命令行参数

```
用法: stealth_mem -k <root_key> -p <pid> [操作]

必填参数:
  -k <root_key>     SKRoot 的 ROOT 密钥
  -p <pid>          目标进程 PID

操作:
  --maps                    显示内存映射
  --read <addr> -s <size>   读取内存
  --write <addr> -d <hex>   写入内存 (hex格式)
  --inject -c <file>        注入 shellcode 文件
  --hook <addr> -c <file>   在指定地址安装 hook
  --find <module>           查找模块基址
```

## 目录结构

```
proc-mem-inject/
├── CMakeLists.txt
├── build.sh
├── build.bat
├── README.md
├── .gitignore
├── .github/workflows/build.yml
├── deps/
│   └── kernel_root_kit/
│       ├── include/          # SKRoot Lite 头文件
│       └── lib/              # SKRoot Lite 静态库
├── src/
│   ├── main.cpp              # CLI 入口
│   ├── memory_injector.cpp   # 核心实现
│   └── include/
│       └── memory_injector.h
└── outputs/
    └── arm64-v8a/
        └── stealth_mem
```

## 实战示例

### 修改游戏数值

```bash
# 1. 找到游戏 PID
PID=$(adb shell pidof com.example.game)

# 2. 找到 libil2cpp.so 基址
adb shell /data/local/tmp/stealth_mem -k "key" -p $PID --find libil2cpp.so
# 输出: libil2cpp.so 基址: 0x7200000000

# 3. 计算目标地址 (基址 + 偏移)
# 假设目标函数偏移是 0x123456
TARGET=0x7200123456

# 4. 读取原始指令
adb shell /data/local/tmp/stealth_mem -k "key" -p $PID --read $TARGET -s 16

# 5. 写入修改 (例如返回固定值)
# MOV W0, #999; RET
adb shell /data/local/tmp/stealth_mem -k "key" -p $PID --write $TARGET -d E0CF87D2C0035FD6
```

## 注意事项

1. **仅供学习研究使用**，请勿用于非法用途
2. 需要 SKRoot Lite 补丁的内核支持
3. 目标进程必须是 64 位 (ARM64)
4. 某些游戏可能检测 /proc/pid/mem 访问
5. 内存修改在进程重启后失效

## 依赖

- [SKRoot Lite](https://github.com/user/SKRoot-linuxKernelRoot) - 内核级 ROOT 框架

## License

MIT License
