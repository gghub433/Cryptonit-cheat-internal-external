// language: C++17, file: shared/structs.hpp, target: Android ARM64
// Unity IL2CPP object layout mirrors — Standoff 2
#pragma once
#include "math.hpp"
#include <cstdint>
#include <string>

// IL2CPP object header (every managed object starts with this)
struct Il2CppObject {
    void* klass;   // pointer to Il2CppClass
    void* monitor; // GC monitor
};

// Unity built-in component base
struct Component : Il2CppObject {
    void* gameObject; // +0x10
};

// Unity Transform
struct Transform : Il2CppObject {
    // accessed via il2cpp calls, not direct offset — engine wraps native transform
};

// Bone IDs used in Standoff 2 character skeleton
enum class BoneId : int {
    Hips       = 0,
    Spine      = 1,
    Spine1     = 2,
    Neck       = 3,
    Head       = 4,
    LeftArm    = 5,
    RightArm   = 6,
    LeftForeArm = 7,
    RightForeArm = 8,
    LeftHand   = 9,
    RightHand  = 10,
    LeftUpLeg  = 11,
    RightUpLeg = 12,
    LeftLeg    = 13,
    RightLeg   = 14,
    LeftFoot   = 15,
    RightFoot  = 16,
    Count      = 17
};

// Player team
enum class Team : int {
    None    = 0,
    Counter = 1,
    Terror  = 2
};

// Weapon category
enum class WeaponType : int {
    Knife    = 0,
    Pistol   = 1,
    Rifle    = 2,
    Shotgun  = 3,
    Sniper   = 4,
    SMG      = 5,
    Grenade  = 6,
    LMG      = 7,
    Melee    = 8,
};

// Cached snapshot of a player — filled each frame from game memory
struct PlayerSnapshot {
    uintptr_t addr;        // base address of PlayerController
    bool       valid;
    bool       is_local;
    Team       team;
    float      health;
    float      max_health;
    bool       is_alive;
    bool       is_visible;
    Vector3    position;
    Vector3    head_pos;
    Vector3    feet_pos;
    Vector3    bones[static_cast<int>(BoneId::Count)];
    Vector2    screen_head;
    Vector2    screen_feet;
    float      distance;
    std::string name;
    WeaponType  weapon;
};

// Snapshot of the local player's weapon state
struct WeaponSnapshot {
    uintptr_t addr;
    WeaponType type;
    int        ammo_clip;
    int        ammo_reserve;
    float      fire_rate;    // rounds per second
    float      next_fire_time;
    float      spread;
    float      recoil_x;
    float      recoil_y;
};

// Frame context passed to all feature modules
struct FrameCtx {
    PlayerSnapshot local;
    PlayerSnapshot players[32];
    int            player_count;
    WeaponSnapshot weapon;
    Matrix4x4      view_proj;
    int            screen_w;
    int            screen_h;
    Vector2        crosshair; // screen center
};
