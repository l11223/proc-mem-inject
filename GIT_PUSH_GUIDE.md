# 📤 Git 推送指南

## 🎯 准备推送到 GitHub

### 步骤 1: 检查当前状态

```bash
cd proc-mem-inject

# 查看当前状态
git status

# 查看修改的文件
git diff
```

### 步骤 2: 添加文件

```bash
# 添加所有新文件和修改
git add .

# 或者选择性添加
git add src/kernel_main.cpp
git add src/kernel_memory_injector.cpp
git add src/kernel_memory_injector.h
git add src/rwprocmem33/
git add README_KERNEL.md
git add QUICKSTART.md
git add INTEGRATION_GUIDE.md
git add ACHIEVEMENT.md
git add .github/workflows/build.yml
git add CMakeLists.txt
```

### 步骤 3: 提交更改

```bash
# 提交更改
git commit -m "🚀 重大升级: 集成 rwProcMem33 实现真正的内核级无痕注入

- ✅ 新增 kernel_mem 内核版本 (基于 rwProcMem33 驱动)
- ✅ 保留 stealth_mem 传统版本 (基于 /proc/pid/mem)
- ✅ 实现双版本架构，灵活选择
- ✅ 完整的 rwProcMem33 用户态封装
- ✅ 真正的内核级操作，硬件级内存访问
- ✅ 驱动级隐藏机制，极难被检测
- ✅ Android 15 完美支持
- ✅ 完善的文档体系

核心特性:
- 硬件级读写进程内存
- 驱动级隐藏 (HideKernelModule)
- 权限动态提升 (SetProcessRoot)
- 硬件断点调试
- 隐蔽通信机制
- 几乎无痕迹

文档:
- README_KERNEL.md - 内核版本详细文档
- QUICKSTART.md - 快速开始指南
- INTEGRATION_GUIDE.md - 集成指南
- ACHIEVEMENT.md - 成就总结

这是一次降维打击级别的技术升级！🔥"
```

### 步骤 4: 推送到 GitHub

```bash
# 推送到远程仓库
git push origin main

# 如果是第一次推送或者分支不存在
git push -u origin main
```

### 步骤 5: 验证 GitHub Actions

推送后，GitHub Actions 会自动开始编译：

1. 访问你的仓库页面
2. 点击 "Actions" 标签
3. 查看最新的构建任务
4. 等待构建完成（大约 5-10 分钟）

构建完成后，你可以下载编译好的文件：
- `stealth_mem-arm64-v8a` - 传统版本
- `kernel_mem-arm64-v8a` - 内核版本
- `proc-mem-inject-all-arm64-v8a` - 所有文件

---

## 🔄 如果需要处理 rwProcMem33 子模块

### 方案 A: 不作为子模块（推荐）

rwProcMem33 已经完整下载到 `deps/rwProcMem33/`，可以直接提交：

```bash
# 确保 rwProcMem33 被包含
git add deps/rwProcMem33/

# 提交
git commit -m "添加 rwProcMem33 依赖"
git push
```

### 方案 B: 作为 Git 子模块

如果想要作为子模块管理：

```bash
# 删除现有目录
rm -rf deps/rwProcMem33

# 添加为子模块
git submodule add https://github.com/abcz316/rwProcMem33.git deps/rwProcMem33

# 提交子模块配置
git add .gitmodules deps/rwProcMem33
git commit -m "添加 rwProcMem33 作为子模块"
git push
```

---

## 📊 推送后的检查清单

### ✅ 代码检查
- [ ] 所有新文件都已添加
- [ ] 提交信息清晰明确
- [ ] 没有提交敏感信息（密钥、密码等）
- [ ] .gitignore 正确配置

### ✅ GitHub Actions 检查
- [ ] Actions 构建成功
- [ ] 两个版本都编译成功
- [ ] 可以下载编译产物
- [ ] BUILD_INFO.txt 正确生成

### ✅ 文档检查
- [ ] README.md 更新
- [ ] README_KERNEL.md 完整
- [ ] QUICKSTART.md 清晰
- [ ] 所有链接正确

---

## 🎉 推送成功后

### 1. 更新 README Badge

在 README.md 顶部添加构建状态徽章：

```markdown
[![Build](https://github.com/l11223/proc-mem-inject/actions/workflows/build.yml/badge.svg)](https://github.com/l11223/proc-mem-inject/actions/workflows/build.yml)
```

### 2. 创建 Release

如果想要创建正式版本：

```bash
# 创建标签
git tag -a v2.0.0 -m "v2.0.0 - 内核级无痕注入重大升级"

# 推送标签
git push origin v2.0.0
```

然后在 GitHub 上创建 Release：
1. 进入仓库的 "Releases" 页面
2. 点击 "Create a new release"
3. 选择刚才创建的标签
4. 填写 Release 说明
5. 上传编译好的文件
6. 发布

### 3. 分享给社区

- 在 README 中添加使用示例
- 创建详细的使用教程
- 分享到技术社区

---

## 🔧 常见问题

### Q1: 推送被拒绝？

```bash
# 先拉取远程更改
git pull origin main --rebase

# 解决冲突后推送
git push origin main
```

### Q2: 文件太大无法推送？

```bash
# 检查大文件
find . -type f -size +50M

# 如果有大文件，添加到 .gitignore
echo "large_file.bin" >> .gitignore
```

### Q3: GitHub Actions 构建失败？

1. 检查 CMakeLists.txt 配置
2. 查看 Actions 日志
3. 确保所有依赖文件都已提交
4. 验证 NDK 版本兼容性

### Q4: 想要撤销提交？

```bash
# 撤销最后一次提交（保留更改）
git reset --soft HEAD~1

# 撤销最后一次提交（丢弃更改）
git reset --hard HEAD~1

# 强制推送（谨慎使用）
git push -f origin main
```

---

## 📝 提交信息规范

使用清晰的提交信息：

```bash
# 功能添加
git commit -m "✨ feat: 添加内核级注入功能"

# Bug 修复
git commit -m "🐛 fix: 修复内存读取错误"

# 文档更新
git commit -m "📝 docs: 更新使用文档"

# 性能优化
git commit -m "⚡ perf: 优化内存访问性能"

# 代码重构
git commit -m "♻️ refactor: 重构适配层代码"

# 测试相关
git commit -m "✅ test: 添加单元测试"

# 构建相关
git commit -m "👷 ci: 更新 GitHub Actions 配置"
```

---

## 🎯 快速命令

```bash
# 一键提交推送（确保已经 git add）
git commit -m "更新内容" && git push

# 查看提交历史
git log --oneline --graph --decorate --all

# 查看远程仓库
git remote -v

# 查看分支
git branch -a
```

---

**虎虎大王，准备好推送了吗？** 🚀

执行这些命令，让全世界看到我们的无痕注入系统！💪✨