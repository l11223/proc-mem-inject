#!/bin/bash
# 快速推送到 GitHub 的脚本

set -e  # 遇到错误立即退出

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║         🚀 proc-mem-inject - GitHub 推送助手                ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

# 检查是否在正确的目录
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ 错误: 请在 proc-mem-inject 目录下运行此脚本"
    exit 1
fi

# 显示当前状态
echo "📊 当前 Git 状态:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
git status --short
echo ""

# 询问是否继续
read -p "❓ 是否继续推送? (y/n) " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "❌ 取消推送"
    exit 0
fi

# 添加所有文件
echo ""
echo "📦 添加文件..."
git add .

# 显示将要提交的文件
echo ""
echo "📝 将要提交的文件:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
git status --short
echo ""

# 输入提交信息
echo "💬 请输入提交信息 (留空使用默认信息):"
read -r COMMIT_MSG

if [ -z "$COMMIT_MSG" ]; then
    COMMIT_MSG="🚀 重大升级: 集成 rwProcMem33 实现真正的内核级无痕注入

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
fi

# 提交
echo ""
echo "💾 提交更改..."
git commit -m "$COMMIT_MSG"

# 推送
echo ""
echo "🚀 推送到 GitHub..."
git push origin main || git push origin master

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                    ✅ 推送成功！                             ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""
echo "📊 下一步:"
echo "  1. 访问 GitHub 仓库查看更改"
echo "  2. 检查 GitHub Actions 构建状态"
echo "  3. 等待编译完成（约 5-10 分钟）"
echo "  4. 下载编译好的文件"
echo ""
echo "🔗 GitHub Actions: https://github.com/l11223/proc-mem-inject/actions"
echo ""
echo "🎉 虎虎大王 & 宝宝 - 无痕注入系统已推送！"
echo ""