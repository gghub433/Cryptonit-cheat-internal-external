// language: C++17, file: internal/src/hooks.hpp
#pragma once
#include <cstdint>

struct RaycastArgs;
struct MiscConfig;

bool hooks_install(uintptr_t il2cpp_base);
void hooks_uninstall();
void hooks_set_misc_config(MiscConfig* cfg);
void hooks_set_silent_aim_ray(RaycastArgs* args);

extern bool g_jump_requested;
