@echo off
REM proc-mem-inject 编译脚本 (Windows)

if "%ANDROID_NDK_HOME%"=="" (
    echo 请设置 ANDROID_NDK_HOME 环境变量
    exit /b 1
)

echo === Building proc-mem-inject ===
echo NDK: %ANDROID_NDK_HOME%

REM 创建输出目录
if not exist build\arm64-v8a mkdir build\arm64-v8a
if not exist outputs\arm64-v8a mkdir outputs\arm64-v8a

REM 编译 arm64-v8a
echo.
echo ^>^>^> Building arm64-v8a...
cd build\arm64-v8a
cmake ..\.. ^
    -G "Ninja" ^
    -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake ^
    -DANDROID_ABI=arm64-v8a ^
    -DANDROID_PLATFORM=android-24 ^
    -DCMAKE_BUILD_TYPE=Release
cmake --build . -j%NUMBER_OF_PROCESSORS%
cd ..\..

echo.
echo === Build Complete ===
echo Output: outputs\arm64-v8a\stealth_mem
