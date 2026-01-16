#!/system/bin/sh
# DNF/王者荣耀 内存工具 - 交互式菜单 (反检测增强版)
# 用法: sh dnf.sh

# ========== 配置 ==========
ROOT_KEY="GD5IyCe0opOqirn6Qs1qDNVFWqpmYc3cNAd9pOgJ8erzOpMf"
PACKAGE="com.tencent.tmgp.dnf"
TOOL="/data/local/tmp/stealth_mem"
SU="/data/GD5IyCe0opOqirn6/su"

# 反检测配置
STEALTH="--stealth"
FAKE_NAME="kworker/0:0"
# ==========================

clear 2>/dev/null || true

echo "======================================"
echo "    内存工具 v2.0 (反检测增强版)"
echo "======================================"
echo ""

# 检查工具是否存在
if [ ! -f "$TOOL" ]; then
    echo "[!] 错误: 找不到 $TOOL"
    echo "[!] 请先推送 stealth_mem 到手机"
    echo ""
    echo "按回车退出..."
    read _
    exit 1
fi

# 检查 su 是否存在
if [ ! -f "$SU" ]; then
    echo "[!] 错误: 找不到 $SU"
    echo ""
    echo "按回车退出..."
    read _
    exit 1
fi

echo "[+] 工具检查通过"
echo "[+] 反检测: 已启用"
echo "[+] 伪装进程名: $FAKE_NAME"
echo ""

# 获取 PID
get_pid() {
    PID=$($SU -c "pidof $PACKAGE" 2>/dev/null)
    if [ -z "$PID" ]; then
        echo "[!] 游戏未运行，请先打开游戏"
        return 1
    fi
    echo "[+] 找到进程: $PID"
    return 0
}

# 等待用户按键
wait_key() {
    echo ""
    echo "按回车继续..."
    read _
}

# 主循环
while true; do
    echo ""
    echo "======================================"
    echo "  1. 显示内存映射"
    echo "  2. 查找 libil2cpp.so (Unity)"
    echo "  3. 查找 libUE4.so (王者/UE4)"
    echo "  4. 查找 libtersafe.so (腾讯安全)"
    echo "  5. 查找其他模块"
    echo "  6. 读取内存"
    echo "  7. 写入内存"
    echo "  8. 切换游戏 (DNF/王者)"
    echo "  9. 切换反检测 开/关"
    echo "  0. 退出"
    echo "======================================"
    printf "请选择 [0-9]: "
    read choice

    case "$choice" in
        1)
            if get_pid; then
                echo "[*] 正在获取内存映射..."
                $TOOL -k "$ROOT_KEY" -p $PID $STEALTH --fake-name "$FAKE_NAME" --maps
            fi
            wait_key
            ;;
        2)
            if get_pid; then
                echo "[*] 正在查找 libil2cpp.so..."
                $TOOL -k "$ROOT_KEY" -p $PID $STEALTH --fake-name "$FAKE_NAME" --find libil2cpp.so
            fi
            wait_key
            ;;
        3)
            if get_pid; then
                echo "[*] 正在查找 libUE4.so..."
                $TOOL -k "$ROOT_KEY" -p $PID $STEALTH --fake-name "$FAKE_NAME" --find libUE4.so
            fi
            wait_key
            ;;
        4)
            if get_pid; then
                echo "[*] 正在查找 libtersafe.so..."
                $TOOL -k "$ROOT_KEY" -p $PID $STEALTH --fake-name "$FAKE_NAME" --find libtersafe.so
            fi
            wait_key
            ;;
        5)
            printf "请输入模块名: "
            read module
            if [ -n "$module" ] && get_pid; then
                $TOOL -k "$ROOT_KEY" -p $PID $STEALTH --fake-name "$FAKE_NAME" --find "$module"
            fi
            wait_key
            ;;
        6)
            printf "请输入地址 (如 0x7000123456): "
            read addr
            printf "读取大小 (默认64): "
            read size
            [ -z "$size" ] && size=64
            if [ -n "$addr" ] && get_pid; then
                $TOOL -k "$ROOT_KEY" -p $PID $STEALTH --fake-name "$FAKE_NAME" --read "$addr" -s $size
            fi
            wait_key
            ;;
        7)
            printf "请输入地址 (如 0x7000123456): "
            read addr
            printf "请输入数据 (如 1F2003D5): "
            read data
            if [ -n "$addr" ] && [ -n "$data" ] && get_pid; then
                $TOOL -k "$ROOT_KEY" -p $PID $STEALTH --fake-name "$FAKE_NAME" --write "$addr" -d "$data"
            fi
            wait_key
            ;;
        8)
            echo ""
            echo "选择游戏:"
            echo "  1. DNF (com.tencent.tmgp.dnf)"
            echo "  2. 王者荣耀 (com.tencent.tmgp.sgame)"
            printf "请选择: "
            read game
            case "$game" in
                1) PACKAGE="com.tencent.tmgp.dnf"; echo "[+] 已切换到 DNF" ;;
                2) PACKAGE="com.tencent.tmgp.sgame"; echo "[+] 已切换到 王者荣耀" ;;
                *) echo "[!] 无效选择" ;;
            esac
            wait_key
            ;;
        9)
            if [ "$STEALTH" = "--stealth" ]; then
                STEALTH="--no-stealth"
                echo "[!] 反检测: 已禁用 (速度更快但风险更高)"
            else
                STEALTH="--stealth"
                echo "[+] 反检测: 已启用"
            fi
            wait_key
            ;;
        0)
            echo "再见宝宝~ "
            exit 0
            ;;
        *)
            echo "[!] 无效选择，请输入 0-9"
            wait_key
            ;;
    esac
done
