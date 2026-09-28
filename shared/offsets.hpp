// language: C++17, file: shared/offsets.hpp, target: Android ARM64 / iOS ARM64
// Standoff 2 v0.39.2 — derived from:
//   - jigganigs22-code/StandoffCheat (il2cpp_resolver.mm + esp.mm source)
//   - Bibizyanov/Standoff2Dumper (OFFSETS.md)
//   - ZailoxTT/Standoff-Dumps (0.37.x dump.cs cross-referenced)
// Dynamic field resolution used where offsets shift per patch.
// Re-derive static offsets: run Il2CppDumper on libil2cpp.so + global-metadata.dat
// then run scripts/offset_finder.py against the new dump.cs / il2cpp.h
#pragma once
#include <cstdint>

// ---- Real Axlebolt namespaces (confirmed from source RE) ----
namespace NS {
    constexpr auto Player       = "Axlebolt.Standoff.Player";
    constexpr auto Inventory    = "Axlebolt.Standoff.Inventory";
    constexpr auto InventoryGun = "Axlebolt.Standoff.Inventory.Gun";
    constexpr auto InventoryWpn = "Axlebolt.Standoff.Inventory.Weapon";
    constexpr auto Controls     = "Axlebolt.Standoff.Controls";
    constexpr auto GameUI       = "Axlebolt.Standoff.Game.UI";
    constexpr auto Game         = "Axlebolt.Standoff.Game";
    constexpr auto Common       = "Axlebolt.Standoff.Common";
    constexpr auto Standoff     = "Axlebolt.Standoff";
    constexpr auto Utilities    = "Axlebolt.Utilities";
    constexpr auto Cam          = "Axlebolt.Cam";
    constexpr auto Ballistics   = "Axlebolt.Ballistics.Simulation";
    constexpr auto Photon       = "Photon";
    constexpr auto Unity        = "UnityEngine";
    constexpr auto Empty        = "";
}

// ---- Real class names (confirmed from il2cpp_resolver.mm source RE) ----
// Used with FindClassRecursive() which tries all namespaces above
namespace Classes {
    // Player domain
    constexpr auto PlayerController         = "PlayerController";
    constexpr auto PlayerManager            = "PlayerManager";
    constexpr auto BipedMap                 = "BipedMap";         // bone positions
    constexpr auto PlayerCharacterView      = "PlayerCharacterView";
    constexpr auto PlayerView               = "PlayerView";
    constexpr auto PlayerInputs             = "PlayerInputs";

    // Weapon domain
    constexpr auto WeaponController         = "WeaponController";
    constexpr auto WeaponManager            = "WeaponManager";
    constexpr auto RecoilControl            = "RecoilControl";
    constexpr auto RecoilParameters         = "RecoilParameters";
    constexpr auto Raycaster                = "Raycaster";
    constexpr auto ShootArea                = "ShootArea";

    // Camera/UI
    constexpr auto CameraScopeZoomer        = "CameraScopeZoomer";
    constexpr auto AimView                  = "AimView";
    constexpr auto HUDView                  = "HUDView";
    constexpr auto FpsOverlay               = "FpsOverlay";
    constexpr auto SpectatorCameraEffect    = "SpectatorCameraEffect";

    // Network
    constexpr auto PhotonPlayerGameExtension = "PhotonPlayerGameExtension";
    constexpr auto PhotonView               = "PhotonView";
    constexpr auto PhotonPlayer             = "PhotonPlayer";

    // Unity built-ins
    constexpr auto GameObject               = "GameObject";
    constexpr auto Transform                = "Transform";
    constexpr auto Camera                   = "Camera";
    constexpr auto MonoBehaviour            = "MonoBehaviour";
    constexpr auto Component                = "Component";
    constexpr auto Object                   = "Object";
    constexpr auto Animator                 = "Animator";
}

// ---- Dynamic field names (try each candidate until one resolves) ----
// Source: esp.mm from jigganigs22-code/StandoffCheat — multi-candidate approach
// for resilience across versions
namespace Fields {
    // PlayerController — health
    constexpr const char* Health[]    = {"_hp","_health","health","Health","_currentHp","hp", nullptr};
    constexpr const char* MaxHealth[] = {"_maxHp","_maxHealth","maxHealth","MaxHealth","_maxHp", nullptr};
    // PlayerController — team
    constexpr const char* Team[]      = {"_team","team","Team","playerTeam","_teamId","TeamId", nullptr};
    // PlayerController — alive state
    constexpr const char* IsAlive[]   = {"_alive","isAlive","IsAlive","_isAlive","alive", nullptr};
    // PlayerController — player name (string)
    constexpr const char* PlayerName[]= {"_playerName","playerName","PlayerName","_name","name","nickName","NickName","_nick", nullptr};
    // PlayerController — weapon controller ref
    constexpr const char* WeaponCtrl[]= {"_weaponController","weaponController","WeaponController","_weapCtrl", nullptr};
    // PlayerController — biped map ref
    constexpr const char* BipedRef[]  = {"_bipedMap","bipedMap","BipedMap","_biped","biped", nullptr};
    // PlayerController — is local player
    constexpr const char* IsLocal[]   = {"_isLocalPlayer","isLocalPlayer","IsLocalPlayer","_isLocal","isLocal", nullptr};

    // BipedMap — head world position (Vector3)
    constexpr const char* HeadPos[]   = {"headPosition","HeadPosition","_headWorldPosition","headWorldPosition","_headPos","_head","headBone", nullptr};

    // WeaponController — recoil multiplier
    constexpr const char* RecoilMult[]= {"_multiplier","multiplier","RecoilMultiplier","_recoilMultiplier","recoilMultiplier", nullptr};
    constexpr const char* RecoilCurve[]= {"_curve","curve","recoilCurve","_recoilCurve", nullptr};
    // WeaponController — fire rate
    constexpr const char* FireRate[]  = {"_fireRate","fireRate","FireRate","_rateOfFire","rateOfFire", nullptr};
    // WeaponController — spread
    constexpr const char* Spread[]    = {"_spread","spread","Spread","_spreadAngle","spreadAngle", nullptr};
    // WeaponController — ammo
    constexpr const char* AmmoCurrent[]= {"_currentAmmo","currentAmmo","CurrentAmmo","_ammo","ammo","_clipAmmo","clipAmmo", nullptr};
    constexpr const char* AmmoMax[]   = {"_maxAmmo","maxAmmo","MaxAmmo","_clipSize","clipSize","_magazineSize","magazineSize", nullptr};
    // WeaponController — weapon ID (for skin changer)
    constexpr const char* WeaponId[]  = {"_weaponId","weaponId","WeaponId","_id","id","_skinId","skinId", nullptr};

    // PhotonView — owner / viewID
    constexpr const char* ViewOwner[] = {"_owner","owner","Owner","ownerId", nullptr};
    constexpr const char* ViewId[]    = {"viewID","ViewID","_viewId","viewId", nullptr};
}

// ---- IL2CPP metadata structure offsets (libil2cpp.so v0.39.x) ----
// Source: Bibizyanov/Standoff2Dumper OFFSETS.md
// These describe the internal IL2CPP metadata layout, used for the custom dumper
namespace IL2CppMeta {
    constexpr uintptr_t protected_metadata_va  = 0x1B60480;
    constexpr uintptr_t code_registration      = 0xB7C32E0;
    constexpr uintptr_t metadata_registration  = 0xB7C3358;
    constexpr uintptr_t metadata_getter_fn     = 0x639D37C;

    // Header field offsets within protected metadata
    constexpr uintptr_t images_count           = 0x14C;   // = 463
    constexpr uintptr_t type_defs_count        = 0x154;   // = 33762
    constexpr uintptr_t generic_containers     = 0x100;   // = 4021
    constexpr uintptr_t stringliterals_offsets = 0x160;
    constexpr uintptr_t stringliterals_data    = 0x138;
    constexpr uintptr_t stringliterals_count   = 0x0FC;
    constexpr uintptr_t stringliterals_off_cnt = 0x044;

    // Module layout (per-module in code registration)
    constexpr uintptr_t mod_method_ptrs        = 0x30;
    constexpr uintptr_t mod_rgctx_ranges       = 0x38;
    constexpr uintptr_t mod_rgctxRangeCount    = 0x40;
    constexpr uintptr_t mod_rgctxRangeCountOff = 0x58;
    constexpr uintptr_t mod_name               = 0x80;
}

// ---- System.Collections.Generic.List<T> layout ----
namespace ListLayout {
    constexpr uintptr_t items = 0x10;  // T[]* backing array
    constexpr uintptr_t size  = 0x18;  // int32 Count
}

// ---- Il2CppArray layout ----
namespace ArrayLayout {
    constexpr uintptr_t length = 0x18; // int32
    constexpr uintptr_t data   = 0x20; // first element
}
