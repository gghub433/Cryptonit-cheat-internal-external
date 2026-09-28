// language: C++17, file: internal/src/features/misc.cpp, target: Android ARM64
// Miscellaneous feature implementations — memory patches applied per-frame
#include "misc.hpp"
#include "../memory.hpp"
#include "../../../shared/offsets.hpp"
#include <cmath>

// -- No Recoil --
// Standoff 2 stores recoil as two floats in WeaponController.
// We write 0.0f each frame before the engine reads them for camera offset.
static void apply_no_recoil(void* wc) {
    if (!wc) return;
    float zero = 0.f;
    mem_write((uintptr_t)wc + Offsets::WeaponController::recoil_x, &zero, sizeof(zero));
    mem_write((uintptr_t)wc + Offsets::WeaponController::recoil_y, &zero, sizeof(zero));
}

// -- No Spread --
static void apply_no_spread(void* wc) {
    if (!wc) return;
    float zero = 0.f;
    mem_write((uintptr_t)wc + Offsets::WeaponController::spread, &zero, sizeof(zero));
}

// -- Rapid Fire --
// Set next_fire_time to 0 so the weapon fires on every frame input is held
static void apply_rapid_fire(void* wc, float mult) {
    if (!wc) return;
    float rate = mem_read<float>((uintptr_t)wc + Offsets::WeaponController::fire_rate);
    if (rate > 0.f) {
        float new_rate = rate * mult;
        mem_write((uintptr_t)wc + Offsets::WeaponController::fire_rate, &new_rate, sizeof(new_rate));
    }
    float zero = 0.f;
    mem_write((uintptr_t)wc + Offsets::WeaponController::next_fire, &zero, sizeof(zero));
}

// -- Infinite Ammo --
static void apply_infinite_ammo(void* wc) {
    if (!wc) return;
    int big = 9999;
    mem_write((uintptr_t)wc + Offsets::WeaponController::ammo_clip,    &big, sizeof(big));
    mem_write((uintptr_t)wc + Offsets::WeaponController::ammo_reserve, &big, sizeof(big));
}

// -- No Fall Damage --
// Handled in hooks.cpp via PlayerController.TakeDamage hook — source tag check

// -- Auto Knife --
// Finds nearest enemy within range and triggers melee attack
static void apply_auto_knife(const FrameCtx& frame, const MiscConfig& cfg,
                              void* player_ctrl, void* il2cpp_bridge) {
    if (!frame.local.is_alive || frame.weapon.type != WeaponType::Knife) return;
    float nearest = cfg.auto_knife_range;
    int   target  = -1;
    for (int i = 0; i < frame.player_count; i++) {
        const auto& p = frame.players[i];
        if (!p.valid || !p.is_alive || p.is_local) continue;
        if (p.team == frame.local.team) continue;
        if (p.distance < nearest) { nearest = p.distance; target = i; }
    }
    if (target < 0) return;
    // Trigger melee via method call through bridge — see hooks.cpp g_do_melee flag
    extern bool g_trigger_melee;
    g_trigger_melee = true;
}

// -- No Flash --
// Hook FlashedEffect.UpdateFlashAmount and return 0. See hooks.cpp.
// Here we just provide the config flag read by the hook.

void misc_apply_pre_frame(const FrameCtx& frame, const MiscConfig& cfg,
                          void* weapon_ctrl, void* player_ctrl) {
    if (cfg.no_recoil && weapon_ctrl)     apply_no_recoil(weapon_ctrl);
    if (cfg.no_spread && weapon_ctrl)     apply_no_spread(weapon_ctrl);
    if (cfg.rapid_fire && weapon_ctrl)    apply_rapid_fire(weapon_ctrl, cfg.rapid_fire_mult);
    if (cfg.infinite_ammo && weapon_ctrl) apply_infinite_ammo(weapon_ctrl);
    if (cfg.auto_knife)                   apply_auto_knife(frame, cfg, player_ctrl, nullptr);
}

void misc_apply_post_frame(const FrameCtx& frame, const MiscConfig& cfg) {
    (void)frame; (void)cfg;
    // bhop and speed_hack applied in CharacterController.Move hook (hooks.cpp)
}

bool g_trigger_melee = false;
