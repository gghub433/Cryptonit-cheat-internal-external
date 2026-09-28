cmake_minimum_required(VERSION 3.22)
project(cryptonit_internal CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build as shared library for dlopen injection
add_library(cryptonit_internal SHARED
    src/main.cpp
    src/memory.cpp
    src/hooks.cpp
    src/overlay.cpp
    src/features/esp.cpp
    src/features/aimbot.cpp
    src/features/misc.cpp
    src/features/skin_changer.cpp
)

target_include_directories(cryptonit_internal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/Dobby/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/imgui
)

# Dobby ARM64 hook library
add_library(dobby STATIC IMPORTED)
set_target_properties(dobby PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../deps/Dobby/libs/arm64-v8a/libdobby.a
)

# ImGui with OpenGL3+Android backend
file(GLOB IMGUI_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/imgui/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/imgui/backends/imgui_impl_android.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/imgui/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SRC})
target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/imgui/backends
)

target_link_libraries(cryptonit_internal
    dobby
    imgui
    log
    android
    EGL
    GLESv3
    dl
)

# Strip symbols to reduce detection surface
target_link_options(cryptonit_internal PRIVATE -Wl,--strip-all)

# Hide all symbols except our entry point
target_compile_options(cryptonit_internal PRIVATE
    -fvisibility=hidden
    -ffunction-sections
    -fdata-sections
    -O2
    -fstack-protector-strong
)
set_target_properties(cryptonit_internal PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Mark our entry point as visible
set_source_files_properties(src/main.cpp PROPERTIES
    COMPILE_FLAGS "-DCRYPTONIT_EXPORT=__attribute__((visibility(\"default\")))"
)
