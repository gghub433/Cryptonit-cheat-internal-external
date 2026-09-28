cmake_minimum_required(VERSION 3.22)
project(cryptonit_external CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Standalone executable (root binary) + JNI shared lib for Android service
add_executable(cryptonit_ext
    src/main.cpp
    src/process.cpp
)

add_library(cryptonit_ext_jni SHARED
    src/main.cpp
    src/process.cpp
)

foreach(tgt cryptonit_ext cryptonit_ext_jni)
    target_include_directories(${tgt} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    )
    target_link_libraries(${tgt} log android dl)
    target_compile_options(${tgt} PRIVATE -O2 -fvisibility=hidden)
endforeach()

# JNI lib doesn't have a main()
target_compile_definitions(cryptonit_ext_jni PRIVATE CRYPTONIT_JNI_MODE)
