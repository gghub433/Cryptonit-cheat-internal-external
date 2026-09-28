// language: C++17, file: internal/src/features/skin_changer.cpp
// Skin changer — in-process field write via resolved IL2CPP field offsets
#include "skin_changer.hpp"
#include "../memory.hpp"
#include "../il2cpp/il2cpp_helper.hpp"
#include "../../../shared/offsets.hpp"
#include "../../../shared/structs.hpp"

// Weapon type ranges used to pick which config slot to apply
static bool is_rifle(WeaponType t) {
    return t == WeaponType::Rifle;
}
static bool is_pistol(WeaponType t) {
    return t == WeaponType::Pistol;
}
static bool is_smg(WeaponType t) {
    return t == WeaponType::SMG;
}
static bool is_sniper(WeaponType t) {
    return t == WeaponType::Sniper;
}
static bool is_heavy(WeaponType t) {
    return t == WeaponType::Shotgun || t == WeaponType::LMG;
}
static bool is_knife(WeaponType t) {
    return t == WeaponType::Knife || t == WeaponType::Melee;
}

void skin_changer_apply(const SkinChangerConfig& cfg,
                        void* weapon_ctrl,
                        void* local_player)
{
    if (!cfg.enabled || !weapon_ctrl) return;

    auto& fo = IL2CppBridge::get().offsets;

    // Read current weapon type and existing skin id
    uintptr_t wc   = (uintptr_t)weapon_ctrl;
    int  weapon_id = mem_read<int>(wc + fo.wc_weapon_id);
    int  w_type    = mem_read<int>(wc + Offsets::WeaponController::weapon_type);
    WeaponType wt  = (WeaponType)w_type;

    // Determine target skin id from config based on weapon type
    int target_id = weapon_id; // default: keep current

    if (cfg.knife_enabled  && is_knife(wt))   target_id = cfg.knife_id;
    else if (cfg.rifle_enabled  && is_rifle(wt))  target_id = cfg.rifle_id;
    else if (cfg.pistol_enabled && is_pistol(wt)) target_id = cfg.pistol_id;
    else if (cfg.smg_enabled    && is_smg(wt))    target_id = cfg.smg_id;
    else if (cfg.sniper_enabled && is_sniper(wt)) target_id = cfg.sniper_id;
    else if (cfg.heavy_enabled  && is_heavy(wt))  target_id = cfg.heavy_id;

    if (target_id != weapon_id)
        mem_write<int>(wc + fo.wc_weapon_id, target_id);

    // Glove skin — Inventory slot offset is separate from WeaponController.
    // Gloves are stored in the inventory object on the local player.
    // Field: _gloveId (try candidates at known offset or via field resolution).
    if (cfg.glove_enabled && local_player) {
        uintptr_t lp = (uintptr_t)local_player;

        // Try inventory object: PlayerController -> _inventory -> _gloveId
        // Inventory pointer is adjacent to weapon controller in field layout.
        // Fallback: write directly to the first matching int field near the
        // WeaponController offset that holds a value in the glove ID range.
        uintptr_t inv = mem_read<uintptr_t>(lp + Offsets::PlayerController::weapon_ctrl + sizeof(uintptr_t));
        if (inv) {
            // _gloveId field sits at offset 0x28 in Inventory object (v0.39.x community report)
            constexpr uintptr_t GLOVE_FIELD_OFF = 0x28;
            int cur_glove = mem_read<int>(inv + GLOVE_FIELD_OFF);
            if (cur_glove != cfg.glove_id)
                mem_write<int>(inv + GLOVE_FIELD_OFF, cfg.glove_id);
        }
    }
}
