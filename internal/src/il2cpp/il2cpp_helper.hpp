// language: C++17, file: internal/src/il2cpp/il2cpp_helper.hpp, target: Android ARM64 + iOS ARM64
// IL2CPP runtime bridge — dynamic class/field resolution
// Architecture from: jigganigs22-code/StandoffCheat (il2cpp_resolver.mm)
// Corrected namespaces: Axlebolt.Standoff.Player / .Inventory / .Game etc.
#pragma once
#include <dlfcn.h>
#include <cstring>
#include <string>
#include <unordered_map>
#include <android/log.h>
#include "../../../shared/offsets.hpp"
#include "../../../shared/math.hpp"

#define IL2LOG(...) __android_log_print(ANDROID_LOG_INFO, "CryptonitIL2", __VA_ARGS__)

// Forward declarations matching il2cpp C API
struct Il2CppDomain;
struct Il2CppAssembly;
struct Il2CppImage;
struct Il2CppClass;
struct Il2CppObject;
struct Il2CppFieldInfo;
struct Il2CppMethodInfo;
struct Il2CppType;
struct Il2CppThread;
struct Il2CppException;
struct Il2CppArray;
struct Il2CppString { int32_t len; uint16_t chars[1]; };

// ---- Function pointer declarations ----
#define DECL_FN(ret, name, args) typedef ret (*fn_##name##_t) args; static fn_##name##_t fn_##name

class IL2CppBridge {
public:
    static IL2CppBridge& get() { static IL2CppBridge inst; return inst; }

    bool init() {
        if (m_ready) return true;

        // Try multiple load paths for Android variants
        void* lib = dlopen("libil2cpp.so", RTLD_NOLOAD | RTLD_NOW);
        if (!lib) {
            const char* paths[] = {
                "libil2cpp.so",
                "/data/app/com.axlebolt.standoff2-1/lib/arm64/libil2cpp.so",
                "/data/app/~~*/com.axlebolt.standoff2*/lib/arm64/libil2cpp.so",
                nullptr
            };
            for (int i = 0; paths[i]; i++) {
                lib = dlopen(paths[i], RTLD_NOW | RTLD_GLOBAL);
                if (lib) break;
            }
        }
        if (!lib) { IL2LOG("libil2cpp.so not found"); return false; }

#define RESOLVE(sym) \
        fn_##sym = (fn_##sym##_t)dlsym(lib, "il2cpp_" #sym); \
        if (!fn_##sym) { IL2LOG("missing: il2cpp_" #sym); return false; }

        RESOLVE(domain_get)
        RESOLVE(domain_get_assemblies)
        RESOLVE(assembly_get_image)
        RESOLVE(image_get_name)
        RESOLVE(class_from_name)
        RESOLVE(class_get_field_from_name)
        RESOLVE(class_get_method_from_name)
        RESOLVE(class_get_name)
        RESOLVE(field_get_offset)
        RESOLVE(field_get_value)
        RESOLVE(field_set_value)
        RESOLVE(field_static_get_value)
        RESOLVE(field_static_set_value)
        RESOLVE(runtime_invoke)
        RESOLVE(object_new)
        RESOLVE(object_get_class)
        RESOLVE(thread_attach)
        RESOLVE(string_length)
        RESOLVE(string_chars)
        RESOLVE(array_length)
#undef RESOLVE

        m_domain = fn_domain_get();
        if (!m_domain) { IL2LOG("domain_get failed"); return false; }
        fn_thread_attach(m_domain);

        size_t cnt = 0;
        m_assemblies = fn_domain_get_assemblies(m_domain, &cnt);
        m_assembly_count = cnt;
        if (!m_assemblies || m_assembly_count == 0) { IL2LOG("no assemblies"); return false; }
        IL2LOG("domain ok — %zu assemblies", m_assembly_count);

        return resolve_classes();
    }

    bool is_ready() const { return m_ready; }

    // ---- Class resolution (tries all Axlebolt namespaces) ----
    Il2CppClass* find_class(const char* ns, const char* name) {
        for (size_t i = 0; i < m_assembly_count; i++) {
            Il2CppImage* img = (Il2CppImage*)fn_assembly_get_image(m_assemblies[i]);
            if (!img) continue;
            Il2CppClass* k = (Il2CppClass*)fn_class_from_name(img, ns, name);
            if (k) return k;
        }
        return nullptr;
    }

    Il2CppClass* find_class_any_ns(const char* name) {
        static const char* namespaces[] = {
            NS::Player, NS::Inventory, NS::InventoryGun, NS::InventoryWpn,
            NS::Controls, NS::GameUI, NS::Game, NS::Common,
            NS::Standoff, NS::Utilities, NS::Cam, NS::Ballistics,
            NS::Unity, NS::Empty, nullptr
        };
        for (size_t i = 0; i < m_assembly_count; i++) {
            Il2CppImage* img = (Il2CppImage*)fn_assembly_get_image(m_assemblies[i]);
            if (!img) continue;
            for (int j = 0; namespaces[j]; j++) {
                Il2CppClass* k = (Il2CppClass*)fn_class_from_name(img, namespaces[j], name);
                if (k) return k;
            }
        }
        return nullptr;
    }

    // ---- Field offset resolution with caching ----
    // Tries multiple candidate field names — resilient across versions
    size_t get_field_offset(Il2CppClass* klass, const char* const* candidates) {
        if (!klass) return 0;
        for (int i = 0; candidates[i]; i++) {
            Il2CppFieldInfo* f = (Il2CppFieldInfo*)fn_class_get_field_from_name(klass, candidates[i]);
            if (!f) continue;
            size_t off = fn_field_get_offset(f);
            if (off) return off;
        }
        return 0;
    }

    size_t get_field_offset(Il2CppClass* klass, const char* name) {
        if (!klass || !name) return 0;
        std::string key = std::string(fn_class_get_name(klass)) + "." + name;
        auto it = m_field_cache.find(key);
        if (it != m_field_cache.end()) return it->second;
        Il2CppFieldInfo* f = (Il2CppFieldInfo*)fn_class_get_field_from_name(klass, name);
        if (!f) return 0;
        size_t off = fn_field_get_offset(f);
        m_field_cache[key] = off;
        return off;
    }

    // ---- Method resolution ----
    Il2CppMethodInfo* get_method(Il2CppClass* klass, const char* name, int argc) {
        if (!klass) return nullptr;
        return (Il2CppMethodInfo*)fn_class_get_method_from_name(klass, name, argc);
    }

    // ---- Runtime invoke ----
    Il2CppObject* invoke(Il2CppMethodInfo* method, void* obj, void** params) {
        Il2CppException* exc = nullptr;
        return (Il2CppObject*)fn_runtime_invoke(method, obj, params, (void**)&exc);
    }

    // ---- Field get/set helpers ----
    template<typename T>
    T read_field(void* obj, size_t offset) {
        T v{};
        if (!obj || !offset) return v;
        memcpy(&v, (uint8_t*)obj + offset, sizeof(T));
        return v;
    }

    template<typename T>
    void write_field(void* obj, size_t offset, const T& val) {
        if (!obj || !offset) return;
        memcpy((uint8_t*)obj + offset, &val, sizeof(T));
    }

    // ---- String helper ----
    std::string read_string(void* str_obj) {
        if (!str_obj) return "";
        int len = fn_string_length(str_obj);
        if (len <= 0 || len > 128) return "";
        uint16_t* chars = (uint16_t*)fn_string_chars(str_obj);
        if (!chars) return "";
        std::string out;
        out.reserve(len);
        for (int i = 0; i < len; i++)
            out += (char)(chars[i] < 128 ? chars[i] : '?');
        return out;
    }

    // ---- Resolved class pointers ----
    struct Classes {
        Il2CppClass* PlayerController         = nullptr;
        Il2CppClass* PlayerManager            = nullptr;
        Il2CppClass* BipedMap                 = nullptr;
        Il2CppClass* PlayerCharacterView      = nullptr;
        Il2CppClass* PlayerView               = nullptr;
        Il2CppClass* PlayerInputs             = nullptr;
        Il2CppClass* WeaponController         = nullptr;
        Il2CppClass* WeaponManager            = nullptr;
        Il2CppClass* RecoilControl            = nullptr;
        Il2CppClass* RecoilParameters         = nullptr;
        Il2CppClass* Raycaster                = nullptr;
        Il2CppClass* ShootArea                = nullptr;
        Il2CppClass* CameraScopeZoomer        = nullptr;
        Il2CppClass* AimView                  = nullptr;
        Il2CppClass* HUDView                  = nullptr;
        Il2CppClass* FpsOverlay               = nullptr;
        Il2CppClass* SpectatorCameraEffect    = nullptr;
        Il2CppClass* PhotonView               = nullptr;
        Il2CppClass* PhotonPlayer             = nullptr;
        Il2CppClass* PhotonPlayerGameExtension= nullptr;
        Il2CppClass* GameObject               = nullptr;
        Il2CppClass* Transform                = nullptr;
        Il2CppClass* Camera                   = nullptr;
        Il2CppClass* MonoBehaviour            = nullptr;
        Il2CppClass* Component                = nullptr;
        Il2CppClass* Object                   = nullptr;
        Il2CppClass* Animator                 = nullptr;
    } classes;

    // ---- Cached field offsets (resolved lazily on first access) ----
    struct FieldOffsets {
        // PlayerController
        size_t pc_health = 0, pc_max_health = 0;
        size_t pc_team = 0, pc_is_alive = 0;
        size_t pc_name = 0, pc_weapon_ctrl = 0;
        size_t pc_biped_map = 0, pc_is_local = 0;
        // BipedMap
        size_t bm_head_pos = 0;
        // WeaponController
        size_t wc_fire_rate = 0, wc_spread = 0;
        size_t wc_ammo = 0, wc_max_ammo = 0;
        size_t wc_weapon_id = 0;
        // RecoilControl
        size_t rc_multiplier = 0;
    } offsets;

    bool resolve_field_offsets() {
        auto& C = classes;
        auto& O = offsets;
        if (C.PlayerController) {
            O.pc_health      = get_field_offset(C.PlayerController, Fields::Health);
            O.pc_max_health  = get_field_offset(C.PlayerController, Fields::MaxHealth);
            O.pc_team        = get_field_offset(C.PlayerController, Fields::Team);
            O.pc_is_alive    = get_field_offset(C.PlayerController, Fields::IsAlive);
            O.pc_name        = get_field_offset(C.PlayerController, Fields::PlayerName);
            O.pc_weapon_ctrl = get_field_offset(C.PlayerController, Fields::WeaponCtrl);
            O.pc_biped_map   = get_field_offset(C.PlayerController, Fields::BipedRef);
            O.pc_is_local    = get_field_offset(C.PlayerController, Fields::IsLocal);
        }
        if (C.BipedMap)
            O.bm_head_pos    = get_field_offset(C.BipedMap, Fields::HeadPos);
        if (C.WeaponController) {
            O.wc_fire_rate   = get_field_offset(C.WeaponController, Fields::FireRate);
            O.wc_spread      = get_field_offset(C.WeaponController, Fields::Spread);
            O.wc_ammo        = get_field_offset(C.WeaponController, Fields::AmmoCurrent);
            O.wc_max_ammo    = get_field_offset(C.WeaponController, Fields::AmmoMax);
            O.wc_weapon_id   = get_field_offset(C.WeaponController, Fields::WeaponId);
        }
        if (C.RecoilControl)
            O.rc_multiplier  = get_field_offset(C.RecoilControl, Fields::RecoilMult);

        IL2LOG("offsets: pc_health=0x%zx pc_team=0x%zx bm_head=0x%zx wc_fire=0x%zx rc_mult=0x%zx",
               O.pc_health, O.pc_team, O.bm_head_pos, O.wc_fire_rate, O.rc_multiplier);
        return O.pc_health != 0;
    }

    // ---- Key method pointers ----
    struct Methods {
        Il2CppMethodInfo* Transform_get_position       = nullptr;
        Il2CppMethodInfo* Transform_set_eulerAngles    = nullptr;
        Il2CppMethodInfo* Transform_get_eulerAngles    = nullptr;
        Il2CppMethodInfo* Camera_get_main              = nullptr;
        Il2CppMethodInfo* Camera_WorldToScreenPoint    = nullptr;
        Il2CppMethodInfo* Camera_get_projectionMatrix  = nullptr;
        Il2CppMethodInfo* Camera_get_worldToCameraMatrix = nullptr;
        Il2CppMethodInfo* Object_FindObjectsOfType     = nullptr;
        Il2CppMethodInfo* GameObject_GetComponent      = nullptr;
        Il2CppMethodInfo* GameObject_Find              = nullptr;
    } methods;

private:
    IL2CppBridge() = default;

    bool resolve_classes() {
        auto& C = classes;
        // Standoff 2 Axlebolt classes
        C.PlayerController          = find_class_any_ns(Classes::PlayerController);
        C.PlayerManager             = find_class_any_ns(Classes::PlayerManager);
        C.BipedMap                  = find_class_any_ns(Classes::BipedMap);
        C.PlayerCharacterView       = find_class_any_ns(Classes::PlayerCharacterView);
        C.PlayerView                = find_class_any_ns(Classes::PlayerView);
        C.PlayerInputs              = find_class_any_ns(Classes::PlayerInputs);
        C.WeaponController          = find_class_any_ns(Classes::WeaponController);
        C.WeaponManager             = find_class_any_ns(Classes::WeaponManager);
        C.RecoilControl             = find_class_any_ns(Classes::RecoilControl);
        C.RecoilParameters          = find_class_any_ns(Classes::RecoilParameters);
        C.Raycaster                 = find_class_any_ns(Classes::Raycaster);
        C.ShootArea                 = find_class_any_ns(Classes::ShootArea);
        C.CameraScopeZoomer         = find_class_any_ns(Classes::CameraScopeZoomer);
        C.AimView                   = find_class_any_ns(Classes::AimView);
        C.HUDView                   = find_class_any_ns(Classes::HUDView);
        C.FpsOverlay                = find_class_any_ns(Classes::FpsOverlay);
        C.SpectatorCameraEffect     = find_class_any_ns(Classes::SpectatorCameraEffect);
        C.PhotonPlayerGameExtension = find_class_any_ns(Classes::PhotonPlayerGameExtension);
        // Photon
        C.PhotonView   = find_class(NS::Photon, Classes::PhotonView);
        C.PhotonPlayer = find_class(NS::Empty,  Classes::PhotonPlayer);
        // Unity
        C.GameObject    = find_class(NS::Unity, Classes::GameObject);
        C.Transform     = find_class(NS::Unity, Classes::Transform);
        C.Camera        = find_class(NS::Unity, Classes::Camera);
        C.MonoBehaviour = find_class(NS::Unity, Classes::MonoBehaviour);
        C.Component     = find_class(NS::Unity, Classes::Component);
        C.Object        = find_class(NS::Unity, Classes::Object);
        C.Animator      = find_class(NS::Unity, Classes::Animator);

        IL2LOG("PlayerController=%p  BipedMap=%p  WeaponController=%p  RecoilControl=%p",
               (void*)C.PlayerController, (void*)C.BipedMap,
               (void*)C.WeaponController, (void*)C.RecoilControl);

        int found = (C.PlayerController ? 1:0) + (C.PlayerManager ? 1:0) +
                    (C.BipedMap ? 1:0) + (C.Transform ? 1:0) +
                    (C.Camera ? 1:0) + (C.GameObject ? 1:0);
        if (found < 4) { IL2LOG("too few classes — %d/6", found); return false; }

        // Methods
        if (C.Transform) {
            methods.Transform_get_position    = get_method(C.Transform, "get_position", 0);
            methods.Transform_set_eulerAngles = get_method(C.Transform, "set_eulerAngles", 1);
            methods.Transform_get_eulerAngles = get_method(C.Transform, "get_eulerAngles", 0);
        }
        if (C.Camera) {
            methods.Camera_get_main             = get_method(C.Camera, "get_main", 0);
            methods.Camera_WorldToScreenPoint   = get_method(C.Camera, "WorldToScreenPoint", 1);
            methods.Camera_get_projectionMatrix = get_method(C.Camera, "get_projectionMatrix", 0);
            methods.Camera_get_worldToCameraMatrix = get_method(C.Camera, "get_worldToCameraMatrix", 0);
        }
        if (C.Object) {
            methods.Object_FindObjectsOfType = get_method(C.Object, "FindObjectsOfType", 1);
        }
        if (C.GameObject) {
            methods.GameObject_GetComponent = get_method(C.GameObject, "GetComponent", 1);
            methods.GameObject_Find         = get_method(C.GameObject, "Find", 1);
        }

        resolve_field_offsets();
        m_ready = true;
        IL2LOG("IL2CPP bridge ready");
        return true;
    }

    bool m_ready = false;
    Il2CppDomain*    m_domain          = nullptr;
    Il2CppAssembly** m_assemblies      = nullptr;
    size_t           m_assembly_count  = 0;
    std::unordered_map<std::string, size_t> m_field_cache;

    // Function pointers
    typedef Il2CppDomain* (*fn_domain_get_t)();
    typedef Il2CppAssembly** (*fn_domain_get_assemblies_t)(Il2CppDomain*, size_t*);
    typedef Il2CppImage* (*fn_assembly_get_image_t)(Il2CppAssembly*);
    typedef const char* (*fn_image_get_name_t)(Il2CppImage*);
    typedef Il2CppClass* (*fn_class_from_name_t)(Il2CppImage*, const char*, const char*);
    typedef Il2CppFieldInfo* (*fn_class_get_field_from_name_t)(Il2CppClass*, const char*);
    typedef Il2CppMethodInfo* (*fn_class_get_method_from_name_t)(Il2CppClass*, const char*, int);
    typedef const char* (*fn_class_get_name_t)(Il2CppClass*);
    typedef size_t (*fn_field_get_offset_t)(Il2CppFieldInfo*);
    typedef void (*fn_field_get_value_t)(void*, Il2CppFieldInfo*, void*);
    typedef void (*fn_field_set_value_t)(void*, Il2CppFieldInfo*, const void*);
    typedef void (*fn_field_static_get_value_t)(Il2CppFieldInfo*, void*);
    typedef void (*fn_field_static_set_value_t)(Il2CppFieldInfo*, const void*);
    typedef Il2CppObject* (*fn_runtime_invoke_t)(Il2CppMethodInfo*, void*, void**, void**);
    typedef Il2CppObject* (*fn_object_new_t)(Il2CppClass*);
    typedef Il2CppClass* (*fn_object_get_class_t)(Il2CppObject*);
    typedef Il2CppThread* (*fn_thread_attach_t)(Il2CppDomain*);
    typedef int (*fn_string_length_t)(void*);
    typedef uint16_t* (*fn_string_chars_t)(void*);
    typedef uint32_t (*fn_array_length_t)(void*);

    fn_domain_get_t                fn_domain_get                = nullptr;
    fn_domain_get_assemblies_t     fn_domain_get_assemblies     = nullptr;
    fn_assembly_get_image_t        fn_assembly_get_image        = nullptr;
    fn_image_get_name_t            fn_image_get_name            = nullptr;
    fn_class_from_name_t           fn_class_from_name           = nullptr;
    fn_class_get_field_from_name_t fn_class_get_field_from_name = nullptr;
    fn_class_get_method_from_name_t fn_class_get_method_from_name = nullptr;
    fn_class_get_name_t            fn_class_get_name            = nullptr;
    fn_field_get_offset_t          fn_field_get_offset          = nullptr;
    fn_field_get_value_t           fn_field_get_value           = nullptr;
    fn_field_set_value_t           fn_field_set_value           = nullptr;
    fn_field_static_get_value_t    fn_field_static_get_value    = nullptr;
    fn_field_static_set_value_t    fn_field_static_set_value    = nullptr;
    fn_runtime_invoke_t            fn_runtime_invoke            = nullptr;
    fn_object_new_t                fn_object_new                = nullptr;
    fn_object_get_class_t          fn_object_get_class          = nullptr;
    fn_thread_attach_t             fn_thread_attach             = nullptr;
    fn_string_length_t             fn_string_length             = nullptr;
    fn_string_chars_t              fn_string_chars              = nullptr;
    fn_array_length_t              fn_array_length              = nullptr;
};
