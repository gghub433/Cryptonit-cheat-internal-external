// language: C++17, file: internal/src/features/aimbot.cpp, target: Android ARM64
// Aimbot: FOV lock, smooth aim, silent aim, triggerbot
// Silent aim works by hooking the ray-cast origin/direction before the server
// validates hit — no visible crosshair movement, server confirms hit
#include "aimbot.hpp"
#include <cmath>
#include <limits>

static BoneId bone_id_for_config(AimBone b) {
    switch (b) {
        case AimBone::Head:   return BoneId::Head;
        case AimBone::Neck:   return BoneId::Neck;
        case AimBone::Chest:  return BoneId::Spine1;
        case AimBone::Pelvis: return BoneId::Hips;
        default:              return BoneId::Head;
    }
}

// Returns which bone is closest to screen center (for AimBone::Nearest)
static BoneId nearest_bone(const PlayerSnapshot& p, const FrameCtx& frame) {
    float best = std::numeric_limits<float>::max();
    BoneId best_bone = BoneId::Head;
    const BoneId check[] = {BoneId::Head, BoneId::Neck, BoneId::Spine1, BoneId::Hips};
    for (BoneId bid : check) {
        Vector2 screen;
        if (!world_to_screen(frame.view_proj, p.bones[(int)bid],
                             screen, frame.screen_w, frame.screen_h)) continue;
        float d = calc_fov(frame.crosshair, screen);
        if (d < best) { best = d; best_bone = bid; }
    }
    return best_bone;
}

AimbotResult aimbot_calc(const FrameCtx& frame, const AimbotConfig& cfg) {
    AimbotResult result{};
    if (!cfg.enabled || !frame.local.is_alive) return result;

    float best_fov = cfg.fov;
    int   best_idx = -1;
    Vector2 best_screen;

    for (int i = 0; i < frame.player_count; i++) {
        const PlayerSnapshot& p = frame.players[i];
        if (!p.valid || !p.is_alive || p.is_local) continue;
        if (cfg.team_check && p.team == frame.local.team) continue;
        if (cfg.vis_check  && !p.is_visible) continue;

        BoneId bid = (cfg.target_bone == AimBone::Nearest)
                     ? nearest_bone(p, frame)
                     : bone_id_for_config(cfg.target_bone);

        Vector2 screen;
        if (!world_to_screen(frame.view_proj, p.bones[(int)bid],
                             screen, frame.screen_w, frame.screen_h)) continue;

        float fov = calc_fov(frame.crosshair, screen);
        if (fov < best_fov) {
            best_fov = fov;
            best_idx = i;
            best_screen = screen;
        }
    }

    if (best_idx < 0) return result;

    result.has_target    = true;
    result.target_idx    = best_idx;
    result.screen_target = best_screen;

    // Delta in pixels → degrees
    // Approximate: 90° FOV maps to ~screen_w/2 pixels
    float px_per_deg = frame.screen_w / 90.f;
    result.aim_delta.x = (best_screen.x - frame.crosshair.x) / px_per_deg;
    result.aim_delta.y = (best_screen.y - frame.crosshair.y) / px_per_deg;

    return result;
}

// Apply via IL2CPP Transform.set_eulerAngles on the camera transform
// For silent aim: instead of changing view angles, we patch the raycast
// origin sent in the fire packet — done in hooks.cpp via Physics.Raycast hook
void aimbot_apply(AimbotResult& result, const AimbotConfig& cfg,
                  void* local_transform, void* bridge_ptr) {
    if (!result.has_target || cfg.silent_aim) return; // silent aim applied in hook

    // Smooth delta
    Vector2 delta = smooth_to({0,0}, result.aim_delta, cfg.smooth);

    // We need current euler angles and add delta
    // This is done through the IL2CPP bridge — see hooks.cpp for the camera euler hook
    // Here we write to a shared atomic that the camera update hook reads
    extern float g_aim_delta_x, g_aim_delta_y;
    g_aim_delta_x = delta.x;
    g_aim_delta_y = delta.y;
}

// These are read by the camera update hook each frame
float g_aim_delta_x = 0.f;
float g_aim_delta_y = 0.f;
