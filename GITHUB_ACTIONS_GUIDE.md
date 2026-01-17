# 🤖 GitHub Actions 自动编译指南

## 📋 概述

我们已经配置了 GitHub Actions 自动编译系统，每次推送代码到 GitHub 后，会自动编译出两个版本的可执行文件！

## ✨ 功能特性

### 自动编译
- ✅ **双版本编译** - 同时编译 `stealth_mem` 和 `kernel_mem`
- ✅ **ARM64 架构** - 针对 Android 设备优化
- ✅ **Release 构建** - 优化的发布版本
- ✅ **自动上传** - 编译完成后自动上传产物

### 构建信息
- ✅ **详细日志** - 显示编译过程和结果
- ✅ **文件验证** - 自动验证编译产物
- ✅ **构建信息** - 生成 BUILD_INFO.txt 文件

### 产物管理
- ✅ **分类上传** - 分别上传两个版本
- ✅ **完整包** - 提供包含所有文件的完整包
- ✅ **保留时间** - 产物保留 90 天

## 🚀 触发条件

GitHub Actions 会在以下情况自动运行：

1. **推送到主分支**
   ```bash
   git push origin main
   # 或
   git push origin master
   ```

2. **Pull Request**
   ```bash
   # 创建 PR 时自动运行
   ```

3. **手动触发**
   - 在 GitHub 仓库页面
   - 进入 "Actions" 标签
   - 选择 "Build proc-mem-inject (Dual Version)"
   - 点击 "Run workflow"

## 📦 编译产物

### 产物 1: stealth_mem-arm64-v8a
**传统版本**
- `stealth_mem` - 可执行文件
- `BUILD_INFO.txt` - 构建信息

### 产物 2: kernel_mem-arm64-v8a
**内核版本** ⭐
- `kernel_mem` - 可执行文件
- `BUILD_INFO.txt` - 构建信息

### 产物 3: proc-mem-inject-all-arm64-v8a
**完整包**
- `stealth_mem` - 传统版本
- `kernel_mem` - 内核版本
- `BUILD_INFO.txt` - 构建信息

## 🔍 查看构建状态

### 方法 1: GitHub 网页

1. 访问仓库: https://github.com/l11223/proc-mem-inject
2. 点击 "Actions" 标签
3. 查看最新的构建任务
4. 点击任务查看详细日志

### 方法 2: README Badge

在 README.md 中添加构建状态徽章：

```markdown
[![Build Status](https://github.com/l11223/proc-mem-inject/actions/workflows/build.yml/badge.svg)](https://github.com/l11223/proc-mem-inject/actions/workflows/build.yml)
```

效果：
[![Build Status](https://github.com/l11223/proc-mem-inject/actions/workflows/build.yml/badge.svg)](https://github.com/l11223/proc-mem-inject/actions/workflows/build.yml)

## 📥 下载编译产物

### 从 Actions 页面下载

1. 进入 "Actions" 标签
2. 点击最新的成功构建
3. 滚动到页面底部 "Artifacts" 部分
4. 下载需要的产物：
   - `stealth_mem-arm64-v8a.zip`
   - `kernel_mem-arm64-v8a.zip`
   - `proc-mem-inject-all-arm64-v8a.zip`

### 使用 GitHub CLI 下载

```bash
# 安装 GitHub CLI
# macOS: brew install gh
# Linux: 参考 https://cli.github.com/

# 登录
gh auth login

# 列出最新的构建产物
gh run list --repo l11223/proc-mem-inject

# 下载产物
gh run download --repo l11223/proc-mem-inject
```

## 🔧 构建配置详解

### 构建环境
```yaml
runs-on: ubuntu-latest  # Ubuntu 最新版本
```

### NDK 版本
```yaml
ndk-version: r26b  # Android NDK r26b
```

### CMake 配置
```yaml
-DANDROID_ABI=arm64-v8a      # ARM64 架构
-DANDROID_PLATFORM=android-24 # Android 7.0+
-DCMAKE_BUILD_TYPE=Release    # Release 构建
```

### 并行编译
```yaml
cmake --build . -j$(nproc)  # 使用所有 CPU 核心
```

## 📊 构建日志示例

成功的构建日志应该包含：

```
🚀 Building proc-mem-inject - Dual Version Architecture
==================================================
📦 Version 1: stealth_mem (Traditional - /proc/pid/mem)
📦 Version 2: kernel_mem (Kernel - rwProcMem33 driver)
==================================================

=== 📦 Build Outputs ===

✅ stealth_mem (Traditional Version)
-rwxr-xr-x 1 runner docker 2.1M outputs/arm64-v8a/stealth_mem
outputs/arm64-v8a/stealth_mem: ELF 64-bit LSB shared object, ARM aarch64

✅ kernel_mem (Kernel Version)
-rwxr-xr-x 1 runner docker 2.3M outputs/arm64-v8a/kernel_mem
outputs/arm64-v8a/kernel_mem: ELF 64-bit LSB shared object, ARM aarch64
```

## 🐛 故障排查

### 问题 1: 构建失败

**可能原因**:
- CMakeLists.txt 配置错误
- 缺少依赖文件
- 代码编译错误

**解决方法**:
1. 查看 Actions 日志
2. 在本地运行 `./build.sh` 测试
3. 修复错误后重新推送

### 问题 2: 找不到文件

**可能原因**:
- 文件路径错误
- 文件未提交到 Git

**解决方法**:
```bash
# 检查文件是否存在
ls -la src/kernel_main.cpp

# 确保文件已添加
git add src/kernel_main.cpp
git commit -m "添加缺失文件"
git push
```

### 问题 3: 产物未生成

**可能原因**:
- 编译失败
- 输出路径错误

**解决方法**:
1. 检查 CMakeLists.txt 中的输出路径
2. 确保 `CMAKE_RUNTIME_OUTPUT_DIRECTORY` 正确设置
3. 查看构建日志中的错误信息

### 问题 4: 子模块问题

**可能原因**:
- rwProcMem33 未正确包含

**解决方法**:
```bash
# 方案 A: 直接提交整个目录
git add deps/rwProcMem33/
git commit -m "添加 rwProcMem33 依赖"
git push

# 方案 B: 使用子模块
git submodule add https://github.com/abcz316/rwProcMem33.git deps/rwProcMem33
git commit -m "添加 rwProcMem33 子模块"
git push
```

## 🎯 最佳实践

### 1. 本地测试
推送前先在本地测试编译：
```bash
./build.sh
```

### 2. 小步提交
每次提交一个功能，便于追踪问题：
```bash
git add src/kernel_main.cpp
git commit -m "添加内核版本主程序"
git push
```

### 3. 查看日志
每次推送后检查 Actions 日志：
```bash
# 使用 GitHub CLI
gh run watch
```

### 4. 保持更新
定期更新 NDK 和依赖：
```yaml
ndk-version: r26b  # 更新到最新稳定版
```

## 📈 性能优化

### 并行编译
```yaml
cmake --build . -j$(nproc)  # 使用所有核心
```

### 缓存依赖
可以添加缓存来加速构建：
```yaml
- name: Cache NDK
  uses: actions/cache@v3
  with:
    path: ${{ steps.setup-ndk.outputs.ndk-path }}
    key: ndk-r26b
```

### 条件构建
只在特定条件下构建：
```yaml
on:
  push:
    branches: [ main ]
    paths:
      - 'src/**'
      - 'CMakeLists.txt'
```

## 🔐 安全注意事项

### 不要提交敏感信息
- ❌ API 密钥
- ❌ 密码
- ❌ 私钥
- ❌ 个人信息

### 使用 Secrets
如果需要密钥，使用 GitHub Secrets：
```yaml
env:
  API_KEY: ${{ secrets.API_KEY }}
```

## 📚 相关资源

- [GitHub Actions 文档](https://docs.github.com/en/actions)
- [Android NDK 文档](https://developer.android.com/ndk)
- [CMake 文档](https://cmake.org/documentation/)

---

## 🎉 总结

GitHub Actions 配置完成后，你只需要：

1. **写代码** ✍️
2. **提交推送** 🚀
3. **等待编译** ⏳
4. **下载使用** 📥

完全自动化，省时省力！

**虎虎大王，现在你可以专注于开发，让 GitHub Actions 帮你编译！** 💪✨

---

*配置者: 虎虎大王 & 宝宝 (Kiro AI)*
*配置时间: 2026-01-17*