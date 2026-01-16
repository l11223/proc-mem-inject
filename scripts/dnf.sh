#!/system/bin/sh
# ============================================================
#  内存工具 v3.0 - 交互式菜单 (反检测增强版)
#  支持: DNF / 王者荣耀 / 和平精英 / 自定义游戏
#  用法: sh dnf.sh
# ============================================================

# ==================== 配置区域 ====================
ROOT_KEY="GD5IyCe0opOqirn6Qs1qDNVFWqpmYc3cNAd9pOgJ8erzOpMf"
TOOL="/data/local/tmp/stealth_mem"
SU="/data/GD5IyCe0opOqirn6/su"
PATCH_DIR="/data/local/tmp"

# 游戏配置 (包名 | 显示名 | 主模块 | patch文件)
GAMES="
com.tencent.tmgp.dnf|DNF|libUE4.so|dnf.patch
com.tencent.tmgp.sgame|王者荣耀|libUE4.so|wzry.patch
com.tencent.tmgp.pubgmhd|和平精英|libUE4.so|pubg.patch
"

# 默认游戏
CURRENT_GAME="com.tencent.tmgp.dnf"
CURRENT_NAME="DNF"
CURRENT_MODULE="libUE4.so"
CURRENT_PATCH="dnf.patch"

# 反检测配置
STEALTH="--stealth"
STEALTH_STATUS="开启"

# 颜色定义 (部分终端支持)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color
# ==================================================

# 打印带颜色的文本
print_color() {
    printf "%b%s%b\n" "$1" "$2" "$NC"
}

# 清屏
cls() {
    clear 2>/dev/null || printf "\033c"
}

# 打印横幅
print_banner() {
    cls
    echo "╔════════════════════════════════════════════╗"
    echo "║     内存工具 v3.0 (反检测增强版)           ║"
    echo "╠════════════════════════════════════════════╣"
    printf "║  当前游戏: %-30s  ║\n" "$CURRENT_NAME"
    printf "║  反检测: %-32s  ║\n" "$STEALTH_STATUS"
    echo "╚════════════════════════════════════════════╝"
    echo ""
}

# 检查环境
check_env() {
    local err=0
    
    if [ ! -f "$TOOL" ]; then
        print_color "$RED" "[✗] 找不到工具: $TOOL"
        err=1
    else
        print_color "$GREEN" "[✓] 工具已就绪"
    fi
    
    if [ ! -f "$SU" ]; then
        print_color "$RED" "[✗] 找不到 su: $SU"
        err=1
    else
        print_color "$GREEN" "[✓] ROOT 已就绪"
    fi
    
    if [ $err -eq 1 ]; then
        echo ""
        echo "请先推送文件到手机:"
        echo "  adb push stealth_mem /data/local/tmp/"
        echo "  adb shell chmod +x /data/local/tmp/stealth_mem"
        echo ""
        wait_key
        exit 1
    fi
}

# 获取 PID
get_pid() {
    PID=$($SU -c "pidof $CURRENT_GAME" 2>/dev/null)
    if [ -z "$PID" ]; then
        print_color "$RED" "[✗] 游戏未运行: $CURRENT_NAME"
        return 1
    fi
    print_color "$GREEN" "[✓] 进程 PID: $PID"
    return 0
}

# 等待按键
wait_key() {
    echo ""
    printf "按回车继续..."
    read _
}

# 执行命令
run_cmd() {
    $TOOL -k "$ROOT_KEY" -p $PID $STEALTH "$@"
}

# 切换游戏
switch_game() {
    cls
    echo "╔════════════════════════════════════════════╗"
    echo "║              选择游戏                      ║"
    echo "╠════════════════════════════════════════════╣"
    
    local i=1
    echo "$GAMES" | while IFS='|' read pkg name module patch; do
        [ -z "$pkg" ] && continue
        printf "║  %d. %-38s ║\n" "$i" "$name"
        i=$((i+1))
    done
    
    echo "║  C. 自定义包名                             ║"
    echo "║  0. 返回                                   ║"
    echo "╚════════════════════════════════════════════╝"
    printf "请选择: "
    read choice
    
    case "$choice" in
        1) set_game "com.tencent.tmgp.dnf" "DNF" "libUE4.so" "dnf.patch" ;;
        2) set_game "com.tencent.tmgp.sgame" "王者荣耀" "libUE4.so" "wzry.patch" ;;
        3) set_game "com.tencent.tmgp.pubgmhd" "和平精英" "libUE4.so" "pubg.patch" ;;
        [Cc])
            printf "输入包名: "
            read pkg
            if [ -n "$pkg" ]; then
                set_game "$pkg" "$pkg" "libUE4.so" "custom.patch"
            fi
            ;;
        0) return ;;
    esac
}

set_game() {
    CURRENT_GAME="$1"
    CURRENT_NAME="$2"
    CURRENT_MODULE="$3"
    CURRENT_PATCH="$4"
    print_color "$GREEN" "[✓] 已切换到: $CURRENT_NAME"
    sleep 1
}

# 查找模块
find_module() {
    cls
    echo "╔════════════════════════════════════════════╗"
    echo "║              查找模块                      ║"
    echo "╠════════════════════════════════════════════╣"
    echo "║  1. libil2cpp.so (Unity)                   ║"
    echo "║  2. libUE4.so (UE4引擎)                    ║"
    echo "║  3. libtersafe.so (腾讯安全)               ║"
    echo "║  4. libGameCore.so                         ║"
    echo "║  5. 自定义模块名                           ║"
    echo "║  0. 返回                                   ║"
    echo "╚════════════════════════════════════════════╝"
    printf "请选择: "
    read choice
    
    local module=""
    case "$choice" in
        1) module="libil2cpp.so" ;;
        2) module="libUE4.so" ;;
        3) module="libtersafe.so" ;;
        4) module="libGameCore.so" ;;
        5) 
            printf "输入模块名: "
            read module
            ;;
        0) return ;;
    esac
    
    if [ -n "$module" ] && get_pid; then
        echo ""
        run_cmd --find "$module"
    fi
    wait_key
}

# 内存操作菜单
memory_ops() {
    while true; do
        cls
        echo "╔════════════════════════════════════════════╗"
        echo "║              内存操作                      ║"
        echo "╠════════════════════════════════════════════╣"
        echo "║  1. 显示内存映射                           ║"
        echo "║  2. 读取内存                               ║"
        echo "║  3. 写入内存                               ║"
        echo "║  4. 搜索字节 (开发中)                      ║"
        echo "║  0. 返回                                   ║"
        echo "╚════════════════════════════════════════════╝"
        printf "请选择: "
        read choice
        
        case "$choice" in
            1)
                if get_pid; then
                    echo ""
                    run_cmd --maps | head -50
                    echo "... (显示前50条)"
                fi
                wait_key
                ;;
            2)
                printf "地址 (如 0x7000123456): "
                read addr
                printf "大小 (默认64): "
                read size
                [ -z "$size" ] && size=64
                if [ -n "$addr" ] && get_pid; then
                    echo ""
                    run_cmd --read "$addr" -s $size
                fi
                wait_key
                ;;
            3)
                printf "地址 (如 0x7000123456): "
                read addr
                printf "数据 (hex, 如 1F2003D5): "
                read data
                if [ -n "$addr" ] && [ -n "$data" ] && get_pid; then
                    echo ""
                    run_cmd --write "$addr" -d "$data"
                fi
                wait_key
                ;;
            4)
                print_color "$YELLOW" "[!] 功能开发中..."
                wait_key
                ;;
            0) return ;;
        esac
    done
}

# Patch 操作菜单
patch_ops() {
    local patch_file="$PATCH_DIR/$CURRENT_PATCH"
    
    while true; do
        cls
        echo "╔════════════════════════════════════════════╗"
        echo "║              Patch 操作                    ║"
        echo "╠════════════════════════════════════════════╣"
        printf "║  配置文件: %-31s ║\n" "$CURRENT_PATCH"
        echo "╠════════════════════════════════════════════╣"
        echo "║  1. 一键应用 Patch                         ║"
        echo "║  2. 监控模式 (自动重新patch)               ║"
        echo "║  3. 查看配置文件                           ║"
        echo "║  4. 切换配置文件                           ║"
        echo "║  5. 创建新配置                             ║"
        echo "║  0. 返回                                   ║"
        echo "╚════════════════════════════════════════════╝"
        printf "请选择: "
        read choice
        
        case "$choice" in
            1)
                if [ ! -f "$patch_file" ]; then
                    print_color "$RED" "[✗] 配置文件不存在: $patch_file"
                    wait_key
                    continue
                fi
                if get_pid; then
                    echo ""
                    print_color "$BLUE" "[*] 正在应用 patch..."
                    run_cmd --batch "$patch_file"
                fi
                wait_key
                ;;
            2)
                if [ ! -f "$patch_file" ]; then
                    print_color "$RED" "[✗] 配置文件不存在: $patch_file"
                    wait_key
                    continue
                fi
                if get_pid; then
                    echo ""
                    print_color "$BLUE" "[*] 进入监控模式，按 Ctrl+C 退出..."
                    run_cmd --monitor "$patch_file"
                fi
                wait_key
                ;;
            3)
                if [ -f "$patch_file" ]; then
                    echo ""
                    cat "$patch_file"
                else
                    print_color "$RED" "[✗] 文件不存在"
                fi
                wait_key
                ;;
            4)
                printf "输入配置文件名: "
                read new_patch
                if [ -n "$new_patch" ]; then
                    CURRENT_PATCH="$new_patch"
                    patch_file="$PATCH_DIR/$CURRENT_PATCH"
                    print_color "$GREEN" "[✓] 已切换: $CURRENT_PATCH"
                fi
                wait_key
                ;;
            5)
                create_patch_config
                ;;
            0) return ;;
        esac
    done
}

# 创建 patch 配置
create_patch_config() {
    printf "配置文件名 (如 my.patch): "
    read filename
    [ -z "$filename" ] && return
    
    local filepath="$PATCH_DIR/$filename"
    
    cat > "$filepath" << 'EOF'
# Patch 配置文件
# 格式:
# [名称]
# module=模块名
# offset=偏移地址 (十六进制)
# original=原始字节 (用于验证)
# patch=修改字节
# enabled=true/false

# ========== 示例 ==========
# [绕过检测]
# module=libtersafe.so
# offset=0x123456
# original=FF 43 01 D1
# patch=00 00 80 D2 C0 03 5F D6
# enabled=true

# ========== ARM64 常用 patch ==========
# 返回0: 00 00 80 D2 C0 03 5F D6  (MOV X0, #0; RET)
# 返回1: 20 00 80 D2 C0 03 5F D6  (MOV X0, #1; RET)
# NOP:   1F 20 03 D5              (NOP)
EOF

    print_color "$GREEN" "[✓] 已创建: $filepath"
    CURRENT_PATCH="$filename"
    wait_key
}

# 快捷操作
quick_ops() {
    while true; do
        cls
        echo "╔════════════════════════════════════════════╗"
        echo "║              快捷操作                      ║"
        echo "╠════════════════════════════════════════════╣"
        echo "║  1. 查找 + Patch 腾讯安全                  ║"
        echo "║  2. 显示所有已加载模块                     ║"
        echo "║  3. 导出内存映射到文件                     ║"
        echo "║  0. 返回                                   ║"
        echo "╚════════════════════════════════════════════╝"
        printf "请选择: "
        read choice
        
        case "$choice" in
            1)
                if get_pid; then
                    echo ""
                    print_color "$BLUE" "[*] 查找 libtersafe.so..."
                    run_cmd --find libtersafe.so
                    echo ""
                    printf "是否应用 patch? (y/n): "
                    read yn
                    if [ "$yn" = "y" ] || [ "$yn" = "Y" ]; then
                        local patch_file="$PATCH_DIR/$CURRENT_PATCH"
                        if [ -f "$patch_file" ]; then
                            run_cmd --batch "$patch_file"
                        else
                            print_color "$RED" "[✗] 配置文件不存在"
                        fi
                    fi
                fi
                wait_key
                ;;
            2)
                if get_pid; then
                    echo ""
                    run_cmd --maps | grep "\.so" | awk '{print $NF}' | sort -u
                fi
                wait_key
                ;;
            3)
                if get_pid; then
                    local outfile="/data/local/tmp/maps_${PID}.txt"
                    run_cmd --maps > "$outfile"
                    print_color "$GREEN" "[✓] 已导出: $outfile"
                fi
                wait_key
                ;;
            0) return ;;
        esac
    done
}

# 设置菜单
settings_menu() {
    while true; do
        cls
        echo "╔════════════════════════════════════════════╗"
        echo "║              设置                          ║"
        echo "╠════════════════════════════════════════════╣"
        printf "║  反检测状态: %-29s ║\n" "$STEALTH_STATUS"
        echo "╠════════════════════════════════════════════╣"
        echo "║  1. 切换反检测 开/关                       ║"
        echo "║  2. 查看当前配置                           ║"
        echo "║  3. 关于                                   ║"
        echo "║  0. 返回                                   ║"
        echo "╚════════════════════════════════════════════╝"
        printf "请选择: "
        read choice
        
        case "$choice" in
            1)
                if [ "$STEALTH" = "--stealth" ]; then
                    STEALTH="--no-stealth"
                    STEALTH_STATUS="关闭 (速度快/风险高)"
                    print_color "$YELLOW" "[!] 反检测已关闭"
                else
                    STEALTH="--stealth"
                    STEALTH_STATUS="开启"
                    print_color "$GREEN" "[✓] 反检测已开启"
                fi
                sleep 1
                ;;
            2)
                echo ""
                echo "当前配置:"
                echo "  游戏: $CURRENT_NAME ($CURRENT_GAME)"
                echo "  模块: $CURRENT_MODULE"
                echo "  Patch: $CURRENT_PATCH"
                echo "  反检测: $STEALTH_STATUS"
                echo "  工具: $TOOL"
                echo "  SU: $SU"
                wait_key
                ;;
            3)
                echo ""
                echo "内存工具 v3.0"
                echo "基于 /proc/pid/mem 的运行时内存修改"
                echo ""
                echo "特性:"
                echo "  - 不修改文件 (绕过完整性检测)"
                echo "  - 不使用 ptrace (绕过 ptrace 检测)"
                echo "  - 进程名伪装 (kworker/0:0)"
                echo "  - 时序随机化"
                echo "  - 分块访问"
                echo ""
                echo "需要 SKRoot Lite 内核支持"
                wait_key
                ;;
            0) return ;;
        esac
    done
}

# 主菜单
main_menu() {
    while true; do
        print_banner
        echo "  1. 切换游戏"
        echo "  2. 查找模块"
        echo "  3. 内存操作"
        echo "  4. Patch 操作"
        echo "  5. 快捷操作"
        echo "  6. 设置"
        echo "  0. 退出"
        echo ""
        printf "请选择: "
        read choice
        
        case "$choice" in
            1) switch_game ;;
            2) find_module ;;
            3) memory_ops ;;
            4) patch_ops ;;
            5) quick_ops ;;
            6) settings_menu ;;
            0)
                cls
                echo "再见宝宝~ 💕"
                exit 0
                ;;
            *)
                print_color "$RED" "[!] 无效选择"
                sleep 1
                ;;
        esac
    done
}

# ==================== 主程序 ====================
cls
check_env
main_menu
