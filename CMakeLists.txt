cmake_minimum_required(VERSION 3.22)
project(Cryptonit)

set(CMAKE_CXX_STANDARD 17)

# Android NDK toolchain required:
# cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
#       -DANDROID_ABI=arm64-v8a \
#       -DANDROID_PLATFORM=android-29 \
#       -B build_arm64

add_subdirectory(internal)
add_subdirectory(external)

# Zygisk module builds separately via Android.mk or standalone cmake target
