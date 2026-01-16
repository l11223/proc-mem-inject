#!/system/bin/sh
# ╔═══════════════════════════════════════════════════════════════════════════╗
# ║                     StealthMem Pro v5.0 - 终极版                          ║
# ║  特性: 反检测 | 批量Patch | 实时监控 | 多游戏 | 收藏夹 | 历史记录         ║
# ╚═══════════════════════════════════════════════════════════════════════════╝

VERSION="5.0"

# ══════════════════════════ 核心配置 ══════════════════════════
ROOT_KEY="GD5IyCe0opOqirn6Qs1qDNVFWqpmYc3cNAd9pOgJ8erzOpMf"
TOOL="/data/local/tmp/stealth_mem"
SU="/data/GD5IyCe0opOqirn6/su"
PATCH_DIR="/data/local/tmp/patches"
LOG_FILE="/data/local/tmp/stealth.log"
HISTORY_FILE="/data/local/tmp/stealth_history.txt"
FAVORITES_FILE="/data/local/tmp/stealth_favorites.txt"

GAME_PKG="com.tencent.tmgp.dnf"
GAME_NAME="DNF"
PID=""
STEALTH_MODE="--stealth"
STEALTH_ON=1
LAST_ADDR=""

# ══════════════════════════ 颜色定义 ══════════════════════════
R='\033[1;31m'
G='\033[1;32m'
Y='\033[1;33m'
B='\033[1;34m'
P='\033[1;35m'
C='\033[1;36m'
W='\033[1;37m'
D='\033[0;90m'
N='\033[0m'
BG_R='\033[41m'
BG_G='\033[42m'
BG_B='\033[44m'

# ══════════════════════════ 工具函数 ══════════════════════════
log() { echo "[$(date '+%H:%M:%S')] $1" >> "$LOG_FILE"; }
add_history() { echo "[$(date '+%H:%M:%S')] $1" >> "$HISTORY_FILE"; }
p_ok()   { printf "  ${G}✓${N} %s\n" "$1"; }
p_err()  { printf "  ${R}✗${N} %s\n" "$1"; }
p_warn() { printf "  ${Y}!${N} %s\n" "$1"; }
p_info() { printf "  ${B}*${N} %s\n" "$1"; }
cls() { clear 2>/dev/null || printf "\033[2J\033[H"; }
pause() { printf "\n  ${D}按回车继续...${N}"; read _; }

line() { printf "${C}════════════════════════════════════════════════════════════${N}\n"; }

draw_box() {
    printf "\n${C}╔════════════════════════════════════════════════════════════╗${N}\n"
    printf "${C}║${N} ${W}%-58s${N} ${C}║${N}\n" "$1"
    printf "${C}╠════════════════════════════════════════════════════════════╣${N}\n"
}

draw_box_end() {
    printf "${C}╚════════════════════════════════════════════════════════════╝${N}\n"
}

menu_item() {
    printf "${C}║${N}  ${Y}%s${N}. %-54s ${C}║${N}\n" "$1" "$2"
}

menu_sep() {
    printf "${C}║${N}  ${D}────────────────────────────────────────────────────────${N} ${C}║${N}\n"
}

# ══════════════════════════ 环境检查 ══════════════════════════
init() {
    mkdir -p "$PATCH_DIR"
    touch "$HISTORY_FILE" "$FAVORITES_FILE"
    
    if [ ! -f "$TOOL" ]; then
        p_err "找不到 stealth_mem"
        echo "请执行: adb push stealth_mem /data/local/tmp/"
        exit 1
    fi
    
    if [ ! -f "$SU" ]; then
        p_err "找不到 SKRoot su"
        exit 1
    fi
    
    chmod +x "$TOOL"
    log "=== StealthMem Pro v$VERSION 启动 ==="
}

refresh_pid() {
    PID=$($SU -c "pidof $GAME_PKG" | awk '{print $1}')
}

check_game() {
    refresh_pid
    [ -n "$PID" ]
}

run() {
    $TOOL -k "$ROOT_KEY" -p "$PID" $STEALTH_MODE "$@"
}

# ══════════════════════════ 主横幅 ══════════════════════════
banner() {
    cls
    printf "${P}"
    cat << 'EOF'

   ███████╗████████╗███████╗ █████╗ ██╗  ████████╗██╗  ██╗
   ██╔════╝╚══██╔══╝██╔════╝██╔══██╗██║  ╚══██╔══╝██║  ██║
   ███████╗   ██║   █████╗  ███████║██║     ██║   ███████║
   ╚════██║   ██║   ██╔══╝  ██╔══██║██║     ██║   ██╔══██║
   ███████║   ██║   ███████╗██║  ██║███████╗██║   ██║  ██║
   ╚══════╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚══════╝╚═╝   ╚═╝  ╚═╝
EOF
    printf "${N}"
    printf "${C}   ███╗   ███╗███████╗███╗   ███╗${N}  ${Y}Pro v$VERSION${N}\n"
    printf "${C}   ████╗ ████║██╔════╝████╗ ████║${N}  ${D}Kernel Memory Tool${N}\n"
    printf "${C}   ██╔████╔██║█████╗  ██╔████╔██║${N}\n"
    printf "${C}   ██║╚██╔╝██║██╔══╝  ██║╚██╔╝██║${N}\n"
    printf "${C}   ██║ ╚═╝ ██║███████╗██║ ╚═╝ ██║${N}\n"
    printf "${C}   ╚═╝     ╚═╝╚══════╝╚═╝     ╚═╝${N}\n\n"
    show_status
}

show_status() {
    local gs ps ss
    if check_game; then
        gs="${BG_G}${W} 运行中 ${N}"
        ps="${G}$PID${N}"
    else
        gs="${BG_R}${W} 未运行 ${N}"
        ps="${D}---${N}"
    fi
    
    if [ $STEALTH_ON -eq 1 ]; then
        ss="${G}● ON${N}"
    else
        ss="${R}○ OFF${N}"
    fi
    
    printf "  ${D}┌──────────────────────────────────────────────────────────┐${N}\n"
    printf "  ${D}│${N} ${W}游戏:${N} %-10s %b  ${W}PID:${N} %-8b  ${W}反检测:${N} %b ${D}│${N}\n" "$GAME_NAME" "$gs" "$ps" "$ss"
    printf "  ${D}└──────────────────────────────────────────────────────────┘${N}\n"
}

# ══════════════════════════ 游戏选择 ══════════════════════════
select_game() {
    cls
    draw_box "选择目标游戏"
    menu_item "1" "DNF 地下城与勇士"
    menu_item "2" "王者荣耀"
    menu_item "3" "和平精英"
    menu_item "4" "英雄联盟手游"
    menu_item "5" "原神"
    menu_item "6" "崩坏：星穹铁道"
    menu_item "7" "明日方舟"
    menu_sep
    menu_item "8" "自定义包名"
    menu_item "9" "从运行中选择"
    menu_item "0" "返回"
    draw_box_end
    
    printf "\n  ${W}选择 > ${N}"
    read choice
    
    case "$choice" in
        1) GAME_PKG="com.tencent.tmgp.dnf"; GAME_NAME="DNF" ;;
        2) GAME_PKG="com.tencent.tmgp.sgame"; GAME_NAME="王者荣耀" ;;
        3) GAME_PKG="com.tencent.tmgp.pubgmhd"; GAME_NAME="和平精英" ;;
        4) GAME_PKG="com.tencent.lolm"; GAME_NAME="LOL手游" ;;
        5) GAME_PKG="com.miHoYo.Yuanshen"; GAME_NAME="原神" ;;
        6) GAME_PKG="com.miHoYo.hkrpg"; GAME_NAME="星穹铁道" ;;
        7) GAME_PKG="com.hypergryph.arknights"; GAME_NAME="明日方舟" ;;
        8)
            printf "\n  ${W}输入包名: ${N}"
            read pkg
            [ -n "$pkg" ] && GAME_PKG="$pkg" && GAME_NAME="$pkg"
            ;;
        9) select_running ;;
        0|*) return ;;
    esac
    
    [ "$choice" != "0" ] && [ "$choice" != "9" ] && {
        p_ok "已选择: $GAME_NAME"
        log "切换游戏: $GAME_NAME"
        sleep 1
    }
}

select_running() {
    printf "\n"
    p_info "扫描运行中的游戏..."
    
    local i=0
    for pkg in com.tencent.tmgp.dnf com.tencent.tmgp.sgame com.tencent.tmgp.pubgmhd com.tencent.lolm com.miHoYo.Yuanshen; do
        local p=$($SU -c "pidof $pkg" | awk '{print $1}')
        if [ -n "$p" ]; then
            i=$((i + 1))
            printf "  ${Y}%d${N}. %s ${G}(PID: %s)${N}\n" "$i" "$pkg" "$p"
            eval "pkg_$i=$pkg"
        fi
    done
    
    [ $i -eq 0 ] && { p_warn "未发现运行中的游戏"; pause; return; }
    
    printf "\n  ${W}选择: ${N}"
    read sel
    
    if [ "$sel" -ge 1 ] && [ "$sel" -le "$i" ]; then
        eval "GAME_PKG=\$pkg_$sel"
        GAME_NAME="$GAME_PKG"
        p_ok "已选择: $GAME_NAME"
        sleep 1
    fi
}

# ══════════════════════════ 内存操作 ══════════════════════════
mem_read() {
    check_game || { p_err "游戏未运行"; pause; return; }
    
    printf "\n  ${W}地址 (0x...): ${N}"
    read addr
    [ -z "$addr" ] && return
    
    printf "  ${W}大小 [64]: ${N}"
    read size
    [ -z "$size" ] && size=64
    
    printf "\n"
    p_info "读取: $addr ($size bytes)"
    line
    run --read "$addr" -s "$size"
    line
    
    LAST_ADDR="$addr"
    add_history "读取: $addr"
    pause
}

mem_write() {
    check_game || { p_err "游戏未运行"; pause; return; }
    
    printf "\n  ${W}地址 (0x...): ${N}"
    read addr
    [ -z "$addr" ] && return
    
    printf "  ${W}数据类型:${N}\n"
    printf "    ${Y}1${N}. Hex字节 (如: 00 00 80 3F)\n"
    printf "    ${Y}2${N}. Int32整数\n"
    printf "  ${W}选择 [1]: ${N}"
    read dtype
    [ -z "$dtype" ] && dtype=1
    
    case "$dtype" in
        2)
            printf "  ${W}整数值: ${N}"
            read val
            data=$(printf '%08X' "$val" | sed 's/\(..\)\(..\)\(..\)\(..\)/\4 \3 \2 \1/')
            ;;
        *)
            printf "  ${W}Hex数据: ${N}"
            read data
            ;;
    esac
    
    [ -n "$addr" ] && [ -n "$data" ] && {
        printf "\n"
        p_info "写入: $addr = $data"
        run --write "$addr" -d "$data"
        LAST_ADDR="$addr"
        add_history "写入: $addr = $data"
    }
    pause
}

mem_maps() {
    check_game || { p_err "游戏未运行"; pause; return; }
    
    printf "\n"
    p_info "内存映射 (前50条)"
    line
    run --maps | head -50
    line
    pause
}

find_module() {
    check_game || { p_err "游戏未运行"; pause; return; }
    
    cls
    draw_box "查找模块"
    menu_item "1" "libil2cpp.so (Unity)"
    menu_item "2" "libUE4.so (虚幻)"
    menu_item "3" "libtersafe.so (腾讯安全)"
    menu_item "4" "libGameCore.so"
    menu_item "5" "libc.so"
    menu_sep
    menu_item "6" "自定义模块名"
    menu_item "7" "列出所有SO"
    menu_item "0" "返回"
    draw_box_end
    
    printf "\n  ${W}选择 > ${N}"
    read choice
    
    local module=""
    case "$choice" in
        1) module="libil2cpp.so" ;;
        2) module="libUE4.so" ;;
        3) module="libtersafe.so" ;;
        4) module="libGameCore.so" ;;
        5) module="libc.so" ;;
        6) printf "  ${W}模块名: ${N}"; read module ;;
        7)
            printf "\n"
            p_info "所有SO模块:"
            line
            run --maps | grep "\.so" | awk '{print $NF}' | sort -u
            line
            pause
            return
            ;;
        0|*) return ;;
    esac
    
    [ -n "$module" ] && {
        printf "\n"
        p_info "查找: $module"
        line
        run --find "$module"
        line
        add_history "查找: $module"
        pause
    }
}

mem_menu() {
    while true; do
        banner
        draw_box "内存操作"
        menu_item "1" "读取内存"
        menu_item "2" "写入内存"
        menu_item "3" "内存映射"
        menu_item "4" "查找模块"
        menu_sep
        menu_item "5" "重复上次 ($LAST_ADDR)"
        menu_item "6" "收藏夹"
        menu_item "0" "返回主菜单"
        draw_box_end
        
        printf "\n  ${W}选择 > ${N}"
        read choice
        
        case "$choice" in
            1) mem_read ;;
            2) mem_write ;;
            3) mem_maps ;;
            4) find_module ;;
            5)
                [ -n "$LAST_ADDR" ] && check_game && {
                    run --read "$LAST_ADDR" -s 64
                    pause
                } || { p_warn "无记录或游戏未运行"; pause; }
                ;;
            6) favorites_menu ;;
            0|*) return ;;
        esac
    done
}

# ══════════════════════════ Patch操作 ══════════════════════════
list_patches() {
    printf "\n  ${W}配置文件:${N}\n"
    local count=0
    if [ -d "$PATCH_DIR" ]; then
        for f in "$PATCH_DIR"/*.patch; do
            [ -f "$f" ] || continue
            count=$((count + 1))
            printf "    ${Y}%d${N}. %s\n" "$count" "$(basename "$f")"
        done
    fi
    [ $count -eq 0 ] && printf "    ${D}(无)${N}\n"
}

apply_patch() {
    check_game || { p_err "游戏未运行"; pause; return; }
    
    list_patches
    printf "\n  ${W}文件名: ${N}"
    read pfile
    [ -z "$pfile" ] && return
    
    local path="$PATCH_DIR/$pfile"
    [ ! -f "$path" ] && path="$pfile"
    
    if [ -f "$path" ]; then
        printf "\n"
        p_info "应用: $(basename "$path")"
        line
        run --batch "$path"
        line
        add_history "Patch: $(basename "$path")"
    else
        p_err "文件不存在"
    fi
    pause
}

monitor_patch() {
    check_game || { p_err "游戏未运行"; pause; return; }
    
    list_patches
    printf "\n  ${W}文件名: ${N}"
    read pfile
    [ -z "$pfile" ] && return
    
    local path="$PATCH_DIR/$pfile"
    [ ! -f "$path" ] && path="$pfile"
    
    if [ -f "$path" ]; then
        printf "\n"
        p_warn "监控模式 - Ctrl+C 退出"
        p_info "配置: $(basename "$path")"
        line
        run --monitor "$path"
        line
    else
        p_err "文件不存在"
    fi
    pause
}

create_patch() {
    printf "\n  ${W}文件名 (如 my.patch): ${N}"
    read fname
    [ -z "$fname" ] && return
    
    local path="$PATCH_DIR/$fname"
    
    cat > "$path" << 'PEOF'
# ═══════════════════════════════════════════════════════════════
# Patch 配置文件
# ═══════════════════════════════════════════════════════════════
# 格式:
#   [名称]
#   module=模块名
#   offset=偏移地址
#   original=原始字节 (可选)
#   patch=修改字节
#   enabled=true/false
# ═══════════════════════════════════════════════════════════════

# 示例:
# [绕过检测]
# module=libtersafe.so
# offset=0x123456
# patch=00 00 80 D2 C0 03 5F D6
# enabled=true

# ═══════════════════════════════════════════════════════════════
# ARM64 常用指令:
#   返回0: 00 00 80 D2 C0 03 5F D6  (MOV X0,#0; RET)
#   返回1: 20 00 80 D2 C0 03 5F D6  (MOV X0,#1; RET)
#   NOP:   1F 20 03 D5
# ═══════════════════════════════════════════════════════════════
PEOF

    p_ok "已创建: $path"
    add_history "创建: $fname"
    pause
}

patch_menu() {
    while true; do
        banner
        draw_box "Patch 操作"
        menu_item "1" "应用 Patch"
        menu_item "2" "监控模式 (自动重写)"
        menu_item "3" "查看配置"
        menu_sep
        menu_item "4" "创建配置"
        menu_item "5" "删除配置"
        menu_item "0" "返回主菜单"
        draw_box_end
        
        printf "\n  ${W}选择 > ${N}"
        read choice
        
        case "$choice" in
            1) apply_patch ;;
            2) monitor_patch ;;
            3)
                list_patches
                printf "\n  ${W}查看: ${N}"
                read f
                [ -z "$f" ] && continue
                local p="$PATCH_DIR/$f"
                [ -f "$p" ] && { line; cat "$p"; line; }
                pause
                ;;
            4) create_patch ;;
            5)
                list_patches
                printf "\n  ${W}删除: ${N}"
                read f
                [ -z "$f" ] && continue
                local p="$PATCH_DIR/$f"
                [ -f "$p" ] && { rm -f "$p"; p_ok "已删除"; sleep 1; }
                ;;
            0|*) return ;;
        esac
    done
}

# ══════════════════════════ 收藏夹 ══════════════════════════
favorites_menu() {
    while true; do
        cls
        draw_box "收藏夹"
        
        local count=0
        while IFS='|' read -r name addr note; do
            [ -z "$name" ] && continue
            count=$((count + 1))
            printf "${C}║${N}  ${Y}%d${N}. %-20s ${G}%s${N}\n" "$count" "$name" "$addr"
        done < "$FAVORITES_FILE"
        
        [ $count -eq 0 ] && printf "${C}║${N}  ${D}(空)${N}\n"
        
        menu_sep
        menu_item "a" "添加收藏"
        menu_item "d" "删除收藏"
        menu_item "r" "读取选中"
        menu_item "0" "返回"
        draw_box_end
        
        printf "\n  ${W}选择 > ${N}"
        read choice
        
        case "$choice" in
            a)
                printf "\n  ${W}名称: ${N}"; read name
                printf "  ${W}地址: ${N}"; read addr
                [ -n "$name" ] && [ -n "$addr" ] && {
                    echo "$name|$addr|" >> "$FAVORITES_FILE"
                    p_ok "已添加"
                    sleep 1
                }
                ;;
            d)
                printf "  ${W}删除第几个: ${N}"; read num
                [ -n "$num" ] && {
                    sed -i "${num}d" "$FAVORITES_FILE"
                    p_ok "已删除"
                    sleep 1
                }
                ;;
            r)
                printf "  ${W}读取第几个: ${N}"; read num
                check_game && [ -n "$num" ] && {
                    local addr=$(sed -n "${num}p" "$FAVORITES_FILE" | cut -d'|' -f2)
                    [ -n "$addr" ] && { run --read "$addr" -s 64; pause; }
                }
                ;;
            0|*) return ;;
        esac
    done
}

# ══════════════════════════ 快捷功能 ══════════════════════════
quick_tersafe() {
    check_game || { p_err "游戏未运行"; pause; return; }
    
    printf "\n"
    p_info "查找 libtersafe.so..."
    line
    run --find libtersafe.so
    line
    
    printf "\n${Y}提示:${N}\n"
    printf "  1. 用IDA分析libtersafe.so\n"
    printf "  2. 找到检测函数偏移\n"
    printf "  3. 创建Patch配置\n"
    pause
}

export_maps() {
    check_game || { p_err "游戏未运行"; pause; return; }
    
    local out="/data/local/tmp/maps_${GAME_NAME}_$(date '+%H%M%S').txt"
    p_info "导出中..."
    run --maps > "$out"
    p_ok "已导出: $out"
    add_history "导出: $out"
    pause
}

quick_menu() {
    while true; do
        banner
        draw_box "快捷功能"
        menu_item "1" "腾讯安全分析"
        menu_item "2" "导出内存映射"
        menu_item "3" "刷新游戏状态"
        menu_sep
        menu_item "4" "查看历史"
        menu_item "5" "查看日志"
        menu_item "0" "返回主菜单"
        draw_box_end
        
        printf "\n  ${W}选择 > ${N}"
        read choice
        
        case "$choice" in
            1) quick_tersafe ;;
            2) export_maps ;;
            3)
                refresh_pid
                [ -n "$PID" ] && p_ok "PID: $PID" || p_warn "未运行"
                sleep 1
                ;;
            4)
                printf "\n  ${W}最近操作:${N}\n"
                line
                tail -20 "$HISTORY_FILE"
                line
                pause
                ;;
            5)
                printf "\n  ${W}日志:${N}\n"
                line
                tail -30 "$LOG_FILE"
                line
                pause
                ;;
            0|*) return ;;
        esac
    done
}

# ══════════════════════════ 设置 ══════════════════════════
settings_menu() {
    while true; do
        banner
        draw_box "设置"
        
        if [ $STEALTH_ON -eq 1 ]; then
            menu_item "1" "反检测: ${G}开启${N}"
        else
            menu_item "1" "反检测: ${R}关闭${N}"
        fi
        
        menu_item "2" "查看配置"
        menu_item "3" "清除历史"
        menu_item "4" "清除日志"
        menu_sep
        menu_item "5" "关于"
        menu_item "0" "返回主菜单"
        draw_box_end
        
        printf "\n  ${W}选择 > ${N}"
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
                sleep 1
                ;;
            2)
                printf "\n  ${W}配置:${N}\n"
                line
                printf "  ROOT_KEY: ${D}%s...${N}\n" "${ROOT_KEY%????????????????????????????????}"
                printf "  TOOL: %s\n" "$TOOL"
                printf "  SU: %s\n" "$SU"
                printf "  GAME: %s\n" "$GAME_NAME"
                line
                pause
                ;;
            3) rm -f "$HISTORY_FILE"; touch "$HISTORY_FILE"; p_ok "已清除"; sleep 1 ;;
            4) rm -f "$LOG_FILE"; p_ok "已清除"; sleep 1 ;;
            5) show_about ;;
            0|*) return ;;
        esac
    done
}

show_about() {
    cls
    printf "\n${P}"
    cat << 'EOF'
   ╔═══════════════════════════════════════════════════════════════╗
   ║                                                               ║
   ║   ███████╗████████╗███████╗ █████╗ ██╗  ████████╗██╗  ██╗     ║
   ║   ██╔════╝╚══██╔══╝██╔════╝██╔══██╗██║  ╚══██╔══╝██║  ██║     ║
   ║   ███████╗   ██║   █████╗  ███████║██║     ██║   ███████║     ║
   ║   ╚════██║   ██║   ██╔══╝  ██╔══██║██║     ██║   ██╔══██║     ║
   ║   ███████║   ██║   ███████╗██║  ██║███████╗██║   ██║  ██║     ║
   ║   ╚══════╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚══════╝╚═╝   ╚═╝  ╚═╝     ║
   ║                     M E M O R Y                               ║
   ╠═══════════════════════════════════════════════════════════════╣
EOF
    printf "${N}"
    printf "${C}   ║${N}  版本: ${Y}v$VERSION${N} Pro Edition                                  ${C}║${N}\n"
    printf "${C}   ║${N}                                                               ${C}║${N}\n"
    printf "${C}   ║${N}  ${W}特性:${N}                                                       ${C}║${N}\n"
    printf "${C}   ║${N}    ${G}✓${N} 基于 /proc/pid/mem 内核级访问                          ${C}║${N}\n"
    printf "${C}   ║${N}    ${G}✓${N} 不修改文件 (绕过完整性检测)                            ${C}║${N}\n"
    printf "${C}   ║${N}    ${G}✓${N} 不使用 ptrace (绕过反调试)                             ${C}║${N}\n"
    printf "${C}   ║${N}    ${G}✓${N} 进程名伪装 + 时序随机化                                ${C}║${N}\n"
    printf "${C}   ║${N}                                                               ${C}║${N}\n"
    printf "${C}   ║${N}  ${W}依赖:${N} SKRoot Lite 内核                                     ${C}║${N}\n"
    printf "${C}   ╚═══════════════════════════════════════════════════════════════╝${N}\n"
    pause
}

# ══════════════════════════ 主菜单 ══════════════════════════
main_menu() {
    while true; do
        banner
        draw_box "主菜单"
        menu_item "1" "选择游戏 [$GAME_NAME]"
        menu_item "2" "内存操作"
        menu_item "3" "Patch 操作"
        menu_item "4" "快捷功能"
        menu_item "5" "设置"
        menu_sep
        menu_item "q" "退出"
        draw_box_end
        
        printf "\n  ${W}选择 > ${N}"
        read choice
        
        case "$choice" in
            1) select_game ;;
            2) mem_menu ;;
            3) patch_menu ;;
            4) quick_menu ;;
            5) settings_menu ;;
            q|Q|0)
                cls
                printf "\n${P}"
                cat << 'EOF'
   ╔═══════════════════════════════════════════════════════════════╗
   ║                                                               ║
   ║                      再见宝宝~ 💕                             ║
   ║                                                               ║
   ║                  StealthMem Pro v5.0                          ║
   ║                                                               ║
   ╚═══════════════════════════════════════════════════════════════╝
EOF
                printf "${N}\n"
                log "=== 退出 ==="
                exit 0
                ;;
        esac
    done
}

# ══════════════════════════ 入口 ══════════════════════════
init
main_menu
