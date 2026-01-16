#!/bin/bash
# proc-mem-inject 编译脚本

set -e

if [ -z "$ANDROID_NDK_HOME" ]; then
    echo "请设置 ANDROID_NDK_HOME 环境变量"
    exit 1
fi

echo "=== Building proc-mem-inject ==="
echo "NDK: $ANDROID_NDK_HOME"

# 创建输出目录
mkdir -p build/arm64-v8a outputs/arm64-v8a

# 编译 arm64-v8a
echo ""
echo ">>> Building arm64-v8a..."
cd build/arm64-v8a
cmake ../.. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
cd ../..

echo ""
echo "=== Build Complete ==="
echo "Output: outputs/arm64-v8a/stealth_mem"
