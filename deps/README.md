# 📦 开源依赖库管理

## 🎯 阶段一：基础依赖库

### 1. android-procmem
**功能**: 简化 `/proc` 文件系统操作
**仓库**: https://github.com/ele7enxxh/android-procmem
**用途**: 
- 解析 `/proc/pid/maps` 获取进程内存映射
- 定位进程内存区域和权限
- 替代手动解析 maps 文件

**集成方式**:
```bash
# 下载到 deps/android-procmem/
git submodule add https://github.com/ele7enxxh/android-procmem.git deps/android-procmem
```

### 2. libroot  
**功能**: Magisk 官方 root 权限验证库
**仓库**: https://github.com/topjohnwu/libroot
**用途**:
- 确保获取完整 root 权限
- 避免半 root 状态导致的权限不足
- Magisk 官方支持，兼容性最好

**集成方式**:
```bash
# 下载到 deps/libroot/
git submodule add https://github.com/topjohnwu/libroot.git deps/libroot
```

## 🎯 阶段二：内核适配依赖库

### 3. android-kernel-offset-finder
**功能**: Android 15 内核函数地址查找
**仓库**: https://github.com/franciscofranco/android-kernel-offset-finder  
**用途**:
- 通过特征码搜索内核函数 (如 `find_task_by_pid`)
- 获取 `task_struct` 结构体偏移
- 替代直接读取 `/proc/kallsyms`

### 4. kallsyms-lookup-android
**功能**: Android 15 符号查找适配
**仓库**: https://github.com/akamai/kallsyms-lookup-android
**用途**:
- 通过 `kallsyms_lookup_name` 间接获取内核函数
- 专门适配 Android 15 的跳板函数获取
- 兜底方案，当特征码搜索失败时使用

### 5. rwProcMem33 (逻辑复用)
**功能**: 内核态内存读写核心逻辑
**仓库**: https://github.com/abcz316/rwProcMem33
**用途**:
- 提取 "内核态内存读写 + 间接函数调用" 代码
- 适配 Android 内核接口 (将 Linux PC 的 insmod 依赖改为 Android root 直接调用)
- 复用其 "分块读写 + 时序随机化" 反检测逻辑

### 6. linux-kernel-call-android
**功能**: Android 用户态间接调用内核函数
**仓库**: https://github.com/rayootech/linux-kernel-call-android
**用途**:
- 绕开 `CONFIG_STRICT_MODULE_RWX` 限制
- 实现用户态安全调用内核函数
- 避免直接系统调用被拦截

## 🎯 阶段三：无痕优化依赖库

### 7. procfs-tools
**功能**: 进程伪装和 FD 清理
**来源**: Linux 内核工具集
**用途**:
- 简化 `/proc/self/comm` 修改 (伪装成 kworker)
- 遍历关闭无用文件描述符
- 清理进程操作痕迹

### 8. libptrace-android
**功能**: ptrace 痕迹抹除
**仓库**: https://github.com/ivan-vasilev/libptrace-android
**用途**:
- 记录并恢复 `task_struct` 的 `ptrace_attached` 字段
- 避免留下 ptrace 附加痕迹
- 使用 "无痕附加" 分支

## 🎯 阶段四：内核态内存操作依赖库

### 9. linux-kernel-mem-android
**功能**: Android 适配的内核态内存操作库
**仓库**: https://github.com/0x1997/linux-kernel-mem-android
**用途**:
- 封装 `kmap`、`copy_from_user/copy_to_user`
- 直接访问目标进程物理内存
- 内核态操作，无用户态痕迹

### 10. proc-maps-parser
**功能**: 进程内存映射解析
**仓库**: https://github.com/89z/proc-maps-parser
**用途**:
- 解析 `/proc/pid/maps`
- 筛选 "rw-x" 权限的空闲内存 (用于注入指令)
- 避免覆盖目标进程有效代码

## 🎯 阶段五：指令注入执行依赖库

### 11. arm64-assembler
**功能**: ARM64 汇编指令处理
**来源**: LLVM AArch64 AsmParser
**用途**:
- 将汇编指令转为机器码
- 适配 ARM64 架构 (Android 主流)
- 支持复杂指令序列

### 12. task-context-manager-android
**功能**: 任务上下文管理
**仓库**: https://github.com/chenxiaolong/task-context-manager-android
**用途**:
- 备份并恢复目标进程的 PC 寄存器
- 备份注入地址原代码
- 执行后彻底抹除注入痕迹

## 🎯 阶段六：测试与适配依赖库

### 13. android-inject-tester
**功能**: 无痕性测试工具
**仓库**: https://github.com/secure-contexts/android-inject-tester
**用途**:
- 检测工具进程残留
- 检测内存操作痕迹
- 检测内核函数调用日志

### 14. kernel-offsets-db
**功能**: 内核偏移数据库
**仓库**: https://github.com/LineageOS/kernel-offsets-db
**用途**:
- 获取主流 Android 15 机型的 `task_struct` 偏移
- 获取内核函数地址
- 实现动态适配不同机型

### 15. android-anti-cheat-tester
**功能**: 防作弊系统测试
**仓库**: https://github.com/GameGuardian/android-anti-cheat-tester
**用途**:
- 针对手游防作弊 (libtersafe、腾讯天御用户态版)
- 验证绕检测效果
- 测试无痕注入的隐蔽性

---

## 📋 集成优先级

### 🔥 立即集成 (阶段一)
1. `android-procmem` - 基础内存映射解析
2. `libroot` - root 权限验证

### ⚡ 优先集成 (阶段二)  
3. `android-kernel-offset-finder` - 内核函数查找
4. `rwProcMem33` - 核心逻辑复用
5. `linux-kernel-call-android` - 内核函数调用

### 🎯 按需集成 (阶段三-六)
6. 其他库按开发进度逐步集成

---

*依赖库管理文档 - 2026-01-17*