// language: C++17, file: internal/src/features/aimbot.hpp
#pragma once
#include "../../../shared/structs.hpp"

enum class AimBone { Head, Neck, Chest, Pelvis, Nearest };

struct AimbotConfig {
    bool  enabled       = true;
    bool  silent_aim    = true;   // redirect bullet server-side without moving crosshair
    bool  triggerbot    = true;   // auto fire when on target
    float fov           = 80.f;   // max screen-pixel radius to lock on
    float smooth        = 4.f;    // 1 = instant snap, higher = smoother
    AimBone target_bone = AimBone::Head;
    bool  team_check    = true;   // never aim at teammates
    bool  vis_check     = true;   // only aim at visible players
    float trigger_fov   = 15.f;   // pixels for triggerbot
};

struct AimbotResult {
    bool      has_target = false;
    int       target_idx = -1;
    Vector2   aim_delta;          // delta to apply to view angles (degrees)
    Vector2   screen_target;
};

AimbotResult aimbot_calc(const FrameCtx& frame, const AimbotConfig& cfg);
void         aimbot_apply(AimbotResult& result, const AimbotConfig& cfg,
                          void* local_transform, void* il2cpp_bridge);
