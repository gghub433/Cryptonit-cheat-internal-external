// language: C++17, file: internal/src/features/skin_changer.hpp
// Skin changer — writes selected skin ID to weapon/inventory objects in-process
#pragma once
#include <cstdint>

struct SkinChangerConfig {
    bool  enabled         = false;

    // Knife override
    bool  knife_enabled   = false;
    int   knife_id        = 6;    // Karambit default

    // Glove override
    bool  glove_enabled   = false;
    int   glove_id        = 3007; // Sport Gloves | Amphibious default

    // Per-category weapon skin (written when the slot is active)
    bool  rifle_enabled   = false;
    int   rifle_id        = 401;  // AK-47 | Redline

    bool  pistol_enabled  = false;
    int   pistol_id       = 604;  // Desert Eagle | Blaze

    bool  smg_enabled     = false;
    int   smg_id          = 709;  // P90 | Asiimov

    bool  sniper_enabled  = false;
    int   sniper_id       = 801;  // AWP | Asiimov

    bool  heavy_enabled   = false;
    int   heavy_id        = 901;  // Nova | Gila
};

// Called once per reader loop iteration with the WeaponController address and
// the local PlayerController address.
void skin_changer_apply(const SkinChangerConfig& cfg,
                        void* weapon_ctrl,
                        void* local_player);
