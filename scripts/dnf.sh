#!/system/bin/sh
# ============================================================
#  StealthMem v4.0 - 专业内存工具
#  特性: 反检测 | 批量Patch | 实时监控 | 多游戏支持
# ============================================================

VERSION="4.0"

# ==================== 核心配置 ====================
ROOT_KEY="GD5IyCe0opOqirn6Qs1qDNVFWqpmYc3cNAd9pOgJ8erzOpMf"
TOOL="/data/local/tmp/stealth_mem"
SU="/data/GD5IyCe0opOqirn6/su"
PATCH_DIR="/data/local/tmp/patches"
LOG_FILE="/data/local/tmp/stealth.log"

# 当前状态
GAME_PKG="com.tencent.tmgp.dnf"
GAME_NAME="DNF"
PID=""
STEALTH_MODE="--stealth"
STEALTH_ON=1

# ==================== 工具函数 ====================

# 日志
log() {
    echo "[$(date '+%H:%M:%S')] $1" >> "$LOG_FILE"
}

# 带颜色输出
R='\033[1;31m'  # 红
G='\033[1;32m'  # 绿
Y='\033[1;33m'  # 黄
B='\033[1;34m'  # 蓝
C='\033[1;36m'  # 青
W='\033[1;37m'  # 白
N='\033[0m'     # 重置

p_ok()   { printf "${G}[✓]${N} %s\n" "$1"; }
p_err()  { printf "${R}[✗]${N} %s\n" "$1"; }
p_warn() { printf "${Y}[!]${N} %s\n" "$1"; }
p_info() { printf "${B}[*]${N} %s\n" "$1"; }

# 清屏
cls() { clear 2>/dev/null || printf "\033[2J\033[H"; }

# 暂停
pause() {
    printf "\n${W}按回车继续...${N}"
    read _
}

# 分隔线
line() {
    printf "${C}────────────────────────────────────────────${N}\n"
}

# ==================== 环境检查 ====================

init() {
    # 创建目录
    mkdir -p "$PATCH_DIR" 2>/dev/null
    
    # 检查工具
    if [ ! -f "$TOOL" ]; then
        p_err "找不到 stealth_mem"
        echo "请执行: adb push stealth_mem /data/local/tmp/"
        exit 1
    fi
    
    if [ ! -f "$SU" ]; then
        p_err "找不到 SKRoot su"
        exit 1
    fi
    
    chmod +x "$TOOL" 2>/dev/null
    log "=== StealthMem v$VERSION 启动 ==="
}

# 获取PID
refresh_pid() {
    PID=$($SU -c "pidof $GAME_PKG" 2>/dev/null | awk '{print $1}')
}

# 检查游戏状态
check_game() {
    refresh_pid
    if [ -z "$PID" ]; then
        return 1
    fi
    return 0
}

# 执行工具命令
run() {
    $TOOL -k "$ROOT_KEY" -p "$PID" $STEALTH_MODE "$@"
}

# ==================== 界面组件 ====================

# 状态栏
show_status() {
    local status_game status_stealth status_pid
    
    if check_game; then
        status_game="${G}运行中${N}"
        status_pid="${G}$PID${N}"
    else
        status_game="${R}未运行${N}"
        status_pid="${R}---${N}"
    fi
    
    if [ $STEALTH_ON -eq 1 ]; then
        status_stealth="${G}开启${N}"
    else
        status_stealth="${Y}关闭${N}"
    fi
    
    printf "\n"
    printf "  ${W}游戏:${N} %-12s  ${W}PID:${N} %-8b  ${W}反检测:${N} %b\n" "$GAME_NAME" "$status_pid" "$status_stealth"
    line
}

# 主横幅
banner() {
    cls
    printf "${C}"
    cat << 'EOF'
   _____ __             ____  __    __  ___              
  / ___// /____  ____ _/ / /_/ /_  /  |/  /__  ____ ___  
  \__ \/ __/ _ \/ __ `/ / __/ __ \/ /|_/ / _ \/ __ `__ \ 
 ___/ / /_/  __/ /_/ / / /_/ / / / /  / /  __/ / / / / / 
/____/\__/\___/\__,_/_/\__/_/ /_/_/  /_/\___/_/ /_/ /_/  
EOF
    printf "${N}"
    printf "                                        ${Y}v$VERSION${N}\n"
    show_status
}

# ==================== 游戏管理 ====================

select_game() {
    cls
    printf "\n${W}  ══════ 选择游戏 ══════${N}\n\n"
    
    echo "  1. DNF              (com.tencent.tmgp.dnf)"
    echo "  2. 王者荣耀        (com.tencent.tmgp.sgame)"
    echo "  3. 和平精英        (com.tencent.tmgp.pubgmhd)"
    echo "  4. 英雄联盟手游    (com.tencent.lolm)"
    echo "  5. 原神            (com.miHoYo.Yuanshen)"
    echo "  6. 自定义包名"
    echo ""
    echo "  0. 返回"
    
    printf "\n${W}选择 > ${N}"
    read choice
    
    case "$choice" in
        1) GAME_PKG="com.tencent.tmgp.dnf"; GAME_NAME="DNF" ;;
        2) GAME_PKG="com.tencent.tmgp.sgame"; GAME_NAME="王者荣耀" ;;
        3) GAME_PKG="com.tencent.tmgp.pubgmhd"; GAME_NAME="和平精英" ;;
        4) GAME_PKG="com.tencent.lolm"; GAME_NAME="LOL手游" ;;
        5) GAME_PKG="com.miHoYo.Yuanshen"; GAME_NAME="原神" ;;
        6)
            printf "包名: "
            read pkg
            [ -n "$pkg" ] && GAME_PKG="$pkg" && GAME_NAME="$pkg"
            ;;
        0) return ;;
    esac
    
    p_ok "已选择: $GAME_NAME"
    log "切换游戏: $GAME_NAME ($GAME_PKG)"
    sleep 1
}

# ==================== 内存操作 ====================

mem_read() {
    if ! check_game; then
        p_err "游戏未运行"
        pause
        return
    fi
    
    printf "地址 (0x...): "
    read addr
    printf "大小 [64]: "
    read size
    [ -z "$size" ] && size=64
    
    if [ -n "$addr" ]; then
        echo ""
        run --read "$addr" -s "$size"
        log "读取内存: $addr ($size bytes)"
    fi
    pause
}

mem_write() {
    if ! check_game; then
        p_err "游戏未运行"
        pause
        return
    fi
    
    printf "地址 (0x...): "
    read addr
    printf "数据 (hex): "
    read data
    
    if [ -n "$addr" ] && [ -n "$data" ]; then
        echo ""
        run --write "$addr" -d "$data"
        log "写入内存: $addr = $data"
    fi
    pause
}

mem_maps() {
    if ! check_game; then
        p_err "游戏未运行"
        pause
        return
    fi
    
    echo ""
    p_info "获取内存映射..."
    run --maps | head -100
    echo ""
    p_warn "显示前100条，完整映射请导出"
    pause
}

find_module() {
    if ! check_game; then
        p_err "游戏未运行"
        pause
        return
    fi
    
    printf "\n${W}  ══════ 查找模块 ══════${N}\n\n"
    echo "  1. libil2cpp.so     (Unity)"
    echo "  2. libUE4.so        (UE4)"
    echo "  3. libtersafe.so    (腾讯安全)"
    echo "  4. libGameCore.so"
    echo "  5. 自定义"
    echo ""
    printf "${W}选择 > ${N}"
    read choice
    
    local module=""
    case "$choice" in
        1) module="libil2cpp.so" ;;
        2) module="libUE4.so" ;;
        3) module="libtersafe.so" ;;
        4) module="libGameCore.so" ;;
        5) printf "模块名: "; read module ;;
    esac
    
    if [ -n "$module" ]; then
        echo ""
        run --find "$module"
        log "查找模块: $module"
    fi
    pause
}

mem_menu() {
    while true; do
        banner
        printf "${W}  ══════ 内存操作 ══════${N}\n\n"
        echo "  1. 读取内存"
        echo "  2. 写入内存"
        echo "  3. 内存映射"
        echo "  4. 查找模块"
        echo "  5. 列出所有SO"
        echo ""
        echo "  0. 返回"
        printf "\n${W}选择 > ${N}"
        read choice
        
        case "$choice" in
            1) mem_read ;;
            2) mem_write ;;
            3) mem_maps ;;
            4) find_module ;;
            5)
                if check_game; then
                    echo ""
                    run --maps | grep "\.so" | awk '{print $NF}' | sort -u | head -50
                    pause
                else
                    p_err "游戏未运行"
                    pause
                fi
                ;;
            0) return ;;
        esac
    done
}

# ==================== Patch 操作 ====================

list_patches() {
    echo ""
    p_info "可用配置文件:"
    ls -1 "$PATCH_DIR"/*.patch 2>/dev/null | while read f; do
        printf "  - %s\n" "$(basename "$f")"
    done
    [ ! "$(ls -A "$PATCH_DIR"/*.patch 2>/dev/null)" ] && echo "  (无)"
}

apply_patch() {
    if ! check_game; then
        p_err "游戏未运行"
        pause
        return
    fi
    
    list_patches
    printf "\n配置文件名: "
    read pfile
    
    local path="$PATCH_DIR/$pfile"
    [ ! -f "$path" ] && path="$pfile"
    
    if [ -f "$path" ]; then
        echo ""
        p_info "应用 Patch: $path"
        run --batch "$path"
        log "应用Patch: $path"
    else
        p_err "文件不存在: $path"
    fi
    pause
}

monitor_patch() {
    if ! check_game; then
        p_err "游戏未运行"
        pause
        return
    fi
    
    list_patches
    printf "\n配置文件名: "
    read pfile
    
    local path="$PATCH_DIR/$pfile"
    [ ! -f "$path" ] && path="$pfile"
    
    if [ -f "$path" ]; then
        echo ""
        p_info "进入监控模式 (Ctrl+C 退出)"
        p_info "配置: $path"
        echo ""
        run --monitor "$path"
        log "监控模式: $path"
    else
        p_err "文件不存在: $path"
    fi
    pause
}

create_patch() {
    printf "文件名 (如 my.patch): "
    read fname
    [ -z "$fname" ] && return
    
    local path="$PATCH_DIR/$fname"
    
    cat > "$path" << 'PATCHEOF'
# ============================================================
# Patch 配置文件
# ============================================================
# 格式:
#   [名称]
#   module=模块名
#   offset=偏移地址
#   original=原始字节 (可选,用于验证)
#   patch=修改字节
#   enabled=true/false
# ============================================================

# 示例: 让检测函数返回0
# [绕过检测]
# module=libtersafe.so
# offset=0x123456
# original=FF 43 01 D1
# patch=00 00 80 D2 C0 03 5F D6
# enabled=true

# ============================================================
# ARM64 常用指令:
#   返回0: 00 00 80 D2 C0 03 5F D6  (MOV X0,#0; RET)
#   返回1: 20 00 80 D2 C0 03 5F D6  (MOV X0,#1; RET)
#   NOP:   1F 20 03 D5
# ============================================================
PATCHEOF

    p_ok "已创建: $path"
    log "创建配置: $path"
    pause
}

patch_menu() {
    while true; do
        banner
        printf "${W}  ══════ Patch 操作 ══════${N}\n\n"
        echo "  1. 应用 Patch"
        echo "  2. 监控模式"
        echo "  3. 查看配置"
        echo "  4. 创建配置"
        echo "  5. 编辑配置"
        echo ""
        echo "  0. 返回"
        printf "\n${W}选择 > ${N}"
        read choice
        
        case "$choice" in
            1) apply_patch ;;
            2) monitor_patch ;;
            3)
                list_patches
                printf "\n查看文件: "
                read f
                [ -f "$PATCH_DIR/$f" ] && cat "$PATCH_DIR/$f"
                [ -f "$f" ] && cat "$f"
                pause
                ;;
            4) create_patch ;;
            5)
                list_patches
                printf "\n编辑文件: "
                read f
                local p="$PATCH_DIR/$f"
                [ -f "$p" ] && vi "$p" 2>/dev/null || nano "$p" 2>/dev/null
                ;;
            0) return ;;
        esac
    done
}

# ==================== 快捷功能 ====================

quick_tersafe() {
    if ! check_game; then
        p_err "游戏未运行"
        pause
        return
    fi
    
    echo ""
    p_info "查找 libtersafe.so..."
    local base=$(run --find libtersafe.so 2>&1 | grep "基址" | awk '{print $NF}')
    
    if [ -n "$base" ]; then
        p_ok "找到: $base"
        echo ""
        printf "是否应用默认Patch? (y/n): "
        read yn
        if [ "$yn" = "y" ]; then
            # 创建临时patch
            local tmp="/data/local/tmp/.tmp_tersafe.patch"
            cat > "$tmp" << EOF
[tersafe_bypass]
module=libtersafe.so
offset=0x0
patch=C0 03 5F D6
enabled=true
EOF
            p_warn "需要正确的偏移地址才能生效"
            p_info "请用IDA分析后修改配置文件"
        fi
    else
        p_err "未找到 libtersafe.so"
    fi
    pause
}

quick_menu() {
    while true; do
        banner
        printf "${W}  ══════ 快捷功能 ══════${N}\n\n"
        echo "  1. 一键查找腾讯安全"
        echo "  2. 导出内存映射"
        echo "  3. 刷新游戏状态"
        echo "  4. 查看日志"
        echo ""
        echo "  0. 返回"
        printf "\n${W}选择 > ${N}"
        read choice
        
        case "$choice" in
            1) quick_tersafe ;;
            2)
                if check_game; then
                    local out="/data/local/tmp/maps_${GAME_NAME}_${PID}.txt"
                    run --maps > "$out"
                    p_ok "已导出: $out"
                    log "导出映射: $out"
                else
                    p_err "游戏未运行"
                fi
                pause
                ;;
            3)
                refresh_pid
                if [ -n "$PID" ]; then
                    p_ok "PID: $PID"
                else
                    p_warn "游戏未运行"
                fi
                pause
                ;;
            4)
                echo ""
                [ -f "$LOG_FILE" ] && tail -30 "$LOG_FILE" || echo "(无日志)"
                pause
                ;;
            0) return ;;
        esac
    done
}

# ==================== 设置 ====================

settings_menu() {
    while true; do
        banner
        printf "${W}  ══════ 设置 ══════${N}\n\n"
        
        if [ $STEALTH_ON -eq 1 ]; then
            echo "  1. 反检测: ${G}开启${N}"
        else
            echo "  1. 反检测: ${Y}关闭${N}"
        fi
        echo "  2. 查看配置"
        echo "  3. 清除日志"
        echo "  4. 关于"
        echo ""
        echo "  0. 返回"
        printf "\n${W}选择 > ${N}"
        read choice
        
        case "$choice" in
            1)
                if [ $STEALTH_ON -eq 1 ]; then
                    STEALTH_ON=0
                    STEALTH_MODE="--no-stealth"
                    p_warn "反检测已关闭"
                else
                    STEALTH_ON=1
                    STEALTH_MODE="--stealth"
                    p_ok "反检测已开启"
                fi
                log "反检测: $STEALTH_ON"
                sleep 1
                ;;
            2)
                echo ""
                echo "  ROOT_KEY: ${ROOT_KEY:0:20}..."
                echo "  TOOL: $TOOL"
                echo "  SU: $SU"
                echo "  PATCH_DIR: $PATCH_DIR"
                echo "  GAME: $GAME_NAME ($GAME_PKG)"
                pause
                ;;
            3)
                rm -f "$LOG_FILE"
                p_ok "日志已清除"
                sleep 1
                ;;
            4)
                echo ""
                echo "  StealthMem v$VERSION"
                echo "  基于 /proc/pid/mem 的运行时内存修改工具"
                echo ""
                echo "  特性:"
                echo "    - 不修改文件 (绕过完整性检测)"
                echo "    - 不使用 ptrace"
                echo "    - 进程名伪装 (kworker/0:0)"
                echo "    - 时序随机化"
                echo "    - 分块访问"
                echo ""
                echo "  需要 SKRoot Lite 内核支持"
                pause
                ;;
            0) return ;;
        esac
    done
}

# ==================== 主菜单 ====================

main_menu() {
    while true; do
        banner
        printf "${W}  ══════ 主菜单 ══════${N}\n\n"
        echo "  1. 选择游戏"
        echo "  2. 内存操作"
        echo "  3. Patch 操作"
        echo "  4. 快捷功能"
        echo "  5. 设置"
        echo ""
        echo "  0. 退出"
        printf "\n${W}选择 > ${N}"
        read choice
        
        case "$choice" in
            1) select_game ;;
            2) mem_menu ;;
            3) patch_menu ;;
            4) quick_menu ;;
            5) settings_menu ;;
            0)
                cls
                echo ""
                p_info "再见宝宝~ 💕"
                echo ""
                log "=== 退出 ==="
                exit 0
                ;;
        esac
    done
}

# ==================== 入口 ====================
init
main_menu
