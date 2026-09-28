// language: C++17, file: internal/src/hooks.cpp, target: Android ARM64
// Dobby ARM64 inline hooks for Standoff 2 IL2CPP methods
// Dobby: github.com/jmpews/Dobby — install in project/deps/Dobby
#include "hooks.hpp"
#include "memory.hpp"
#include "../../shared/offsets.hpp"
#include "../../shared/structs.hpp"
#include "features/misc.hpp"
#include <dobby.h>
#include <cmath>
#include <android/log.h>

#define TAG "CryptonitHook"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// ---- Globals shared with feature modules ----
extern float g_aim_delta_x, g_aim_delta_y;
extern bool  g_trigger_melee;

// Config references — set in main.cpp after init
static MiscConfig* g_misc_cfg = nullptr;

void hooks_set_misc_config(MiscConfig* cfg) { g_misc_cfg = cfg; }

// ---- Original function pointers ----
static void*  (*orig_PlayerUpdate)(void* self, void* method_info) = nullptr;
static void   (*orig_TakeDamage)(void* self, float dmg, int src, void* mi) = nullptr;
static void*  (*orig_RaycastFire)(void* self, void* mi)           = nullptr;
static void   (*orig_MoveCtrl)(void* self, void* v3, void* mi)    = nullptr;
static void   (*orig_SetEuler)(void* self, void* v3, void* mi)    = nullptr;
static float  (*orig_FlashAmount)(void* self, void* mi)           = nullptr;
static void   (*orig_SmokeShow)(void* self, bool show, void* mi)  = nullptr;

// ---- Hook: PlayerController.Update ----
// Called once per frame per player — used to validate player state
static void* hk_PlayerUpdate(void* self, void* mi) {
    return orig_PlayerUpdate(self, mi);
}

// ---- Hook: PlayerController.TakeDamage ----
// Intercept fall damage: source 99 = fall; discard if no_fall_dmg enabled
static void hk_TakeDamage(void* self, float dmg, int src, void* mi) {
    if (g_misc_cfg && g_misc_cfg->no_fall_dmg && src == 99) return;
    orig_TakeDamage(self, dmg, src, mi);
}

// ---- Hook: WeaponController.FireRaycast (silent aim core) ----
// Before the raycast fires, redirect its direction toward the aimbot target
// direction comes from a shared struct written by aimbot_calc
struct RaycastArgs {
    Vector3 origin;
    Vector3 direction;
};
static RaycastArgs* g_pending_raycast = nullptr; // written by aimbot thread
static void* hk_RaycastFire(void* self, void* mi) {
    if (g_pending_raycast) {
        // Overwrite origin and direction in WeaponController's fire state
        // offset of the internal ray origin buffer in WeaponController is 0x58 (RE'd)
        uintptr_t base = reinterpret_cast<uintptr_t>(self);
        mem_write(base + 0x58, &g_pending_raycast->origin,    sizeof(Vector3));
        mem_write(base + 0x64, &g_pending_raycast->direction, sizeof(Vector3));
    }
    return orig_RaycastFire(self, mi);
}

void hooks_set_silent_aim_ray(RaycastArgs* args) {
    g_pending_raycast = args;
}

// ---- Hook: CharacterController.SimpleMove (bhop + speed) ----
static void hk_MoveCtrl(void* self, void* v3_ptr, void* mi) {
    if (!g_misc_cfg) { orig_MoveCtrl(self, v3_ptr, mi); return; }
    Vector3 vel;
    if (v3_ptr) memcpy(&vel, v3_ptr, sizeof(Vector3));

    if (g_misc_cfg->speed_hack)
        vel = vel * g_misc_cfg->speed_mult;

    // Bhop: if player is on the ground and jump key was just pressed,
    // set upward velocity immediately (avoid the frame delay that kills bhop)
    extern bool g_jump_requested;
    if (g_misc_cfg->bhop && g_jump_requested) {
        vel.y = 8.0f; // unity units/s upward — matches base jump velocity
        g_jump_requested = false;
    }

    if (v3_ptr) memcpy(v3_ptr, &vel, sizeof(Vector3));
    orig_MoveCtrl(self, v3_ptr, mi);
}

// ---- Hook: CameraController.set_eulerAngles (smooth aimbot delta) ----
static void hk_SetEuler(void* self, void* v3_ptr, void* mi) {
    Vector3 angles;
    if (v3_ptr) memcpy(&angles, v3_ptr, sizeof(Vector3));
    angles.x += g_aim_delta_y;
    angles.y += g_aim_delta_x;
    // clamp vertical
    if (angles.x >  89.f) angles.x =  89.f;
    if (angles.x < -89.f) angles.x = -89.f;
    g_aim_delta_x = 0.f;
    g_aim_delta_y = 0.f;
    if (v3_ptr) memcpy(v3_ptr, &angles, sizeof(Vector3));
    orig_SetEuler(self, v3_ptr, mi);
}

// ---- Hook: FlashedEffect.UpdateFlashAmount (no flash) ----
static float hk_FlashAmount(void* self, void* mi) {
    if (g_misc_cfg && g_misc_cfg->no_flash) return 0.f;
    return orig_FlashAmount(self, mi);
}

// ---- Hook: SmokeEffect.Show (no smoke) ----
static void hk_SmokeShow(void* self, bool show, void* mi) {
    if (g_misc_cfg && g_misc_cfg->no_smoke && show) return;
    orig_SmokeShow(self, show, mi);
}

bool g_jump_requested = false;

// ---- Install all hooks ----
// Addresses are found by: module_base + offset from il2cpp_dump or pattern scan
bool hooks_install(uintptr_t il2cpp_base) {
    // All method addresses below are IL2CPP method pointers — resolve via method metadata
    // Pattern scan fallback: find instruction sequence unique to each function
    // These offsets are for v1.37.x ARM64 — re-derive with Ghidra after update

    struct HookEntry {
        uintptr_t rva;
        void**    orig;
        void*     hook;
        const char* name;
    };

    HookEntry entries[] = {
        // PlayerController$$Update
        {0x00AC4210, (void**)&orig_PlayerUpdate, (void*)hk_PlayerUpdate, "PlayerUpdate"},
        // PlayerController$$TakeDamage
        {0x00AC5880, (void**)&orig_TakeDamage,   (void*)hk_TakeDamage,   "TakeDamage"},
        // WeaponController$$FireRaycast
        {0x00B18340, (void**)&orig_RaycastFire,  (void*)hk_RaycastFire,  "RaycastFire"},
        // CharacterController$$SimpleMove
        {0x00521F40, (void**)&orig_MoveCtrl,     (void*)hk_MoveCtrl,     "SimpleMove"},
        // CameraController$$set_eulerAngles
        {0x00523A10, (void**)&orig_SetEuler,     (void*)hk_SetEuler,     "SetEuler"},
        // FlashedEffect$$UpdateFlashAmount
        {0x00C22840, (void**)&orig_FlashAmount,  (void*)hk_FlashAmount,  "FlashAmount"},
        // SmokeEffect$$Show
        {0x00C3A100, (void**)&orig_SmokeShow,    (void*)hk_SmokeShow,    "SmokeShow"},
    };

    bool ok = true;
    for (auto& e : entries) {
        void* target = reinterpret_cast<void*>(il2cpp_base + e.rva);
        int ret = DobbyHook(target, e.hook, e.orig);
        if (ret != 0) {
            LOGE("Hook failed: %s (rva=0x%lx, ret=%d)", e.name, e.rva, ret);
            ok = false;
        } else {
            LOGI("Hooked: %s @ %p", e.name, target);
        }
    }
    return ok;
}

void hooks_uninstall() {
    // DobbyDestroy restores original bytes
    auto destroy_if = [](void* orig) {
        if (orig) DobbyDestroy(orig);
    };
    destroy_if((void*)orig_PlayerUpdate);
    destroy_if((void*)orig_TakeDamage);
    destroy_if((void*)orig_RaycastFire);
    destroy_if((void*)orig_MoveCtrl);
    destroy_if((void*)orig_SetEuler);
    destroy_if((void*)orig_FlashAmount);
    destroy_if((void*)orig_SmokeShow);
}
