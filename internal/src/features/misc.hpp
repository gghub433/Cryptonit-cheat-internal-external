// language: C++17, file: internal/src/features/misc.hpp
#pragma once
#include "../../../shared/structs.hpp"

struct MiscConfig {
    // Weapon mechanics
    bool  no_recoil     = true;
    bool  no_spread     = true;
    bool  rapid_fire    = true;
    float rapid_fire_mult = 3.0f; // fire rate multiplier (e.g. 3x = triple rate)
    bool  infinite_ammo = true;
    bool  no_reload     = false;  // skip reload animation

    // Movement
    bool  bhop          = true;   // auto-bunny hop
    bool  speed_hack    = false;  // server-side visible, use with care
    float speed_mult    = 1.5f;
    bool  no_fall_dmg   = true;

    // Visibility
    bool  chams         = true;   // material replacement for player models
    bool  wallhack_material = false; // render enemies through walls (shader patch)
    bool  fullbright    = false;  // remove fog/darkness

    // Utility
    bool  auto_knife    = true;   // auto-swing knife when enemy in melee range
    float auto_knife_range = 2.5f; // meters
    bool  auto_defuse   = false;  // auto-defuse bomb
    bool  auto_plant    = false;  // auto-plant bomb
    bool  no_flash      = true;   // nullify flashbang effect
    bool  no_smoke      = true;   // remove smoke grenade opacity
};

void misc_apply_pre_frame(const FrameCtx& frame, const MiscConfig& cfg,
                          void* weapon_ctrl, void* player_ctrl);
void misc_apply_post_frame(const FrameCtx& frame, const MiscConfig& cfg);
