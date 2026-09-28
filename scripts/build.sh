#!/bin/bash
# Build script for Cryptonit (Standoff 2 cheat)
# Requirements: Android NDK r25c+, CMake 3.22+
# Usage: ./scripts/build.sh [/path/to/ndk]
set -e

NDK="${1:-$ANDROID_NDK_HOME}"
if [[ -z "$NDK" ]]; then
    echo "ERROR: Set ANDROID_NDK_HOME or pass NDK path as argument"
    exit 1
fi

TOOLCHAIN="$NDK/build/cmake/android.toolchain.cmake"
BUILD_DIR="build_arm64"
ABI="arm64-v8a"
PLATFORM="android-29"

echo "[*] Checking dependencies..."

# Dobby
if [[ ! -f "deps/Dobby/libs/arm64-v8a/libdobby.a" ]]; then
    echo "[*] Cloning and building Dobby..."
    mkdir -p deps && cd deps
    git clone --depth=1 https://github.com/jmpews/Dobby.git
    cd Dobby
    mkdir -p build_arm64 && cd build_arm64
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DANDROID_ABI=$ABI \
        -DANDROID_PLATFORM=$PLATFORM \
        -DDOBBY_DEBUG=OFF \
        -DDOBBY_GENERATE_SHARED=OFF \
        -DCMAKE_BUILD_TYPE=Release
    cmake --build . -j$(nproc)
    mkdir -p ../../libs/arm64-v8a
    cp libdobby.a ../../libs/arm64-v8a/
    cd ../../..
fi

# ImGui
if [[ ! -d "deps/imgui" ]]; then
    echo "[*] Cloning ImGui..."
    mkdir -p deps
    git clone --depth=1 --branch docking https://github.com/ocornut/imgui.git deps/imgui
fi

echo "[*] Configuring..."
mkdir -p $BUILD_DIR
cmake -B $BUILD_DIR \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=$PLATFORM \
    -DCMAKE_BUILD_TYPE=Release \
    -DANDROID_STL=c++_shared

echo "[*] Building..."
cmake --build $BUILD_DIR -j$(nproc)

echo ""
echo "[+] Build complete. Outputs:"
echo "    Internal:  $BUILD_DIR/internal/libcryptonit_internal.so"
echo "    External:  $BUILD_DIR/external/cryptonit_ext"
echo "    Ext JNI:   $BUILD_DIR/external/libcryptonit_ext_jni.so"
echo ""
echo "Deployment:"
echo "  Internal (Zygisk):"
echo "    1. Copy libcryptonit_internal.so to Magisk module /data/adb/modules/cryptonit/"
echo "    2. Flash zygisk_module.zip via Magisk"
echo "  External (root binary):"
echo "    adb push $BUILD_DIR/external/cryptonit_ext /data/local/tmp/"
echo "    adb shell su -c chmod +x /data/local/tmp/cryptonit_ext"
echo "    adb shell su -c /data/local/tmp/cryptonit_ext"
