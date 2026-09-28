// language: C++17, file: internal/src/main.cpp, target: Android ARM64
// Cryptonit Internal — Zygisk module entry + main cheat loop
// Injected into com.axlebolt.standoff2 via Zygisk post-specialize callback
#include <jni.h>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include <atomic>
#include <thread>

#include "memory.hpp"
#include "hooks.hpp"
#include "overlay.hpp"
#include "il2cpp/il2cpp_helper.hpp"
#include "features/esp.hpp"
#include "features/aimbot.hpp"
#include "features/misc.hpp"
#include "features/skin_changer.hpp"
#include "../../shared/skin_ids.hpp"
#include "../../shared/structs.hpp"
#include "../../shared/offsets.hpp"

#define TAG "Cryptonit"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// ---- Global config (modified by ImGui UI) ----
static ESPConfig          g_esp_cfg;
static AimbotConfig       g_aim_cfg;
static MiscConfig         g_misc_cfg;
static SkinChangerConfig  g_skin_cfg;

// ---- Shared frame state ----
static FrameCtx g_frame;
static std::atomic<bool> g_running{true};

// ---- ImGui UI menu ----
static bool g_menu_visible = true; // toggled by volume-down long-press

static void render_menu(int sw, int sh) {
    (void)sw; (void)sh;

    // Toggle with volume button is detected via a motion event hook (not shown)
    if (!g_menu_visible) return;

    ImGui::SetNextWindowSize(ImVec2(380, 520), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Once);
    ImGui::Begin("CRYPTONIT v1.0", &g_menu_visible, ImGuiWindowFlags_NoResize);

    if (ImGui::BeginTabBar("tabs")) {

        // ---- ESP Tab ----
        if (ImGui::BeginTabItem("ESP")) {
            ImGui::Checkbox("Enable ESP",       &g_esp_cfg.enabled);
            ImGui::Checkbox("Boxes",            &g_esp_cfg.boxes);
            ImGui::Checkbox("Skeleton",         &g_esp_cfg.skeleton);
            ImGui::Checkbox("Health Bar",       &g_esp_cfg.health_bar);
            ImGui::Checkbox("Name Tags",        &g_esp_cfg.name_tag);
            ImGui::Checkbox("Distance",         &g_esp_cfg.distance);
            ImGui::Checkbox("Weapon Names",     &g_esp_cfg.weapon_name);
            ImGui::Checkbox("Snaplines",        &g_esp_cfg.snaplines);
            ImGui::Checkbox("Radar",            &g_esp_cfg.radar);
            ImGui::Checkbox("Visible Only",     &g_esp_cfg.visible_only);
            ImGui::ColorEdit3("Enemy Color",    &g_esp_cfg.box_r_enemy);
            ImGui::ColorEdit3("Ally Color",     &g_esp_cfg.box_r_ally);
            ImGui::SliderFloat("Radar Range",   &g_esp_cfg.radar_range,  20.f, 300.f);
            ImGui::EndTabItem();
        }

        // ---- Aimbot Tab ----
        if (ImGui::BeginTabItem("Aimbot")) {
            ImGui::Checkbox("Enable Aimbot",    &g_aim_cfg.enabled);
            ImGui::Checkbox("Silent Aim",       &g_aim_cfg.silent_aim);
            ImGui::Checkbox("Triggerbot",       &g_aim_cfg.triggerbot);
            ImGui::Checkbox("Visibility Check", &g_aim_cfg.vis_check);
            ImGui::Checkbox("Team Check",       &g_aim_cfg.team_check);
            ImGui::SliderFloat("FOV",           &g_aim_cfg.fov,     10.f, 300.f);
            ImGui::SliderFloat("Smooth",        &g_aim_cfg.smooth,   1.f,  20.f);
            ImGui::SliderFloat("Trigger FOV",   &g_aim_cfg.trigger_fov, 5.f, 80.f);
            const char* bones[] = {"Head","Neck","Chest","Pelvis","Nearest"};
            int bi = (int)g_aim_cfg.target_bone;
            if (ImGui::Combo("Target Bone", &bi, bones, 5))
                g_aim_cfg.target_bone = (AimBone)bi;
            ImGui::EndTabItem();
        }

        // ---- Misc Tab ----
        if (ImGui::BeginTabItem("Misc")) {
            ImGui::Text("Weapon");
            ImGui::Checkbox("No Recoil",        &g_misc_cfg.no_recoil);
            ImGui::Checkbox("No Spread",        &g_misc_cfg.no_spread);
            ImGui::Checkbox("Rapid Fire",       &g_misc_cfg.rapid_fire);
            if (g_misc_cfg.rapid_fire)
                ImGui::SliderFloat("Fire Rate x", &g_misc_cfg.rapid_fire_mult, 1.f, 10.f);
            ImGui::Checkbox("Infinite Ammo",    &g_misc_cfg.infinite_ammo);

            ImGui::Separator();
            ImGui::Text("Movement");
            ImGui::Checkbox("Bunny Hop",        &g_misc_cfg.bhop);
            ImGui::Checkbox("Speed Hack",       &g_misc_cfg.speed_hack);
            if (g_misc_cfg.speed_hack)
                ImGui::SliderFloat("Speed x",   &g_misc_cfg.speed_mult, 1.f, 5.f);
            ImGui::Checkbox("No Fall Damage",   &g_misc_cfg.no_fall_dmg);

            ImGui::Separator();
            ImGui::Text("Visual");
            ImGui::Checkbox("No Flash",         &g_misc_cfg.no_flash);
            ImGui::Checkbox("No Smoke",         &g_misc_cfg.no_smoke);

            ImGui::Separator();
            ImGui::Text("Utility");
            ImGui::Checkbox("Auto Knife",       &g_misc_cfg.auto_knife);
            if (g_misc_cfg.auto_knife)
                ImGui::SliderFloat("Knife Range",&g_misc_cfg.auto_knife_range, 1.f, 6.f);
            ImGui::EndTabItem();
        }

        // ---- Skins Tab ----
        if (ImGui::BeginTabItem("Skins")) {
            ImGui::Checkbox("Enable Skin Changer", &g_skin_cfg.enabled);
            ImGui::Separator();

            ImGui::Checkbox("Knife Override",  &g_skin_cfg.knife_enabled);
            if (g_skin_cfg.knife_enabled) {
                static int knife_idx = 0;
                static const char* knife_names[SkinIds::KnivesCount];
                static bool knife_names_built = false;
                if (!knife_names_built) {
                    for (int i = 0; i < SkinIds::KnivesCount; i++)
                        knife_names[i] = SkinIds::Knives[i].name;
                    knife_names_built = true;
                }
                if (ImGui::Combo("Knife", &knife_idx, knife_names, SkinIds::KnivesCount))
                    g_skin_cfg.knife_id = SkinIds::Knives[knife_idx].id;
            }

            ImGui::Checkbox("Glove Override",  &g_skin_cfg.glove_enabled);
            if (g_skin_cfg.glove_enabled) {
                static int glove_idx = 0;
                static const char* glove_names[SkinIds::GlovesCount];
                static bool glove_names_built = false;
                if (!glove_names_built) {
                    for (int i = 0; i < SkinIds::GlovesCount; i++)
                        glove_names[i] = SkinIds::Gloves[i].name;
                    glove_names_built = true;
                }
                if (ImGui::Combo("Gloves", &glove_idx, glove_names, SkinIds::GlovesCount))
                    g_skin_cfg.glove_id = SkinIds::Gloves[glove_idx].id;
            }

            ImGui::Separator();

            ImGui::Checkbox("Rifle Skin",      &g_skin_cfg.rifle_enabled);
            if (g_skin_cfg.rifle_enabled) {
                static int ri = 0;
                static const char* rn[SkinIds::AK47Count];
                static bool rb = false;
                if (!rb) { for (int i=0;i<SkinIds::AK47Count;i++) rn[i]=SkinIds::AK47[i].name; rb=true; }
                if (ImGui::Combo("Rifle", &ri, rn, SkinIds::AK47Count))
                    g_skin_cfg.rifle_id = SkinIds::AK47[ri].id;
            }

            ImGui::Checkbox("Pistol Skin",     &g_skin_cfg.pistol_enabled);
            if (g_skin_cfg.pistol_enabled) {
                static int pi = 0;
                static const char* pn[SkinIds::PistolCount];
                static bool pb = false;
                if (!pb) { for (int i=0;i<SkinIds::PistolCount;i++) pn[i]=SkinIds::Pistol[i].name; pb=true; }
                if (ImGui::Combo("Pistol", &pi, pn, SkinIds::PistolCount))
                    g_skin_cfg.pistol_id = SkinIds::Pistol[pi].id;
            }

            ImGui::Checkbox("SMG Skin",        &g_skin_cfg.smg_enabled);
            if (g_skin_cfg.smg_enabled) {
                static int si = 0;
                static const char* sn[SkinIds::SMGCount];
                static bool sb = false;
                if (!sb) { for (int i=0;i<SkinIds::SMGCount;i++) sn[i]=SkinIds::SMG[i].name; sb=true; }
                if (ImGui::Combo("SMG", &si, sn, SkinIds::SMGCount))
                    g_skin_cfg.smg_id = SkinIds::SMG[si].id;
            }

            ImGui::Checkbox("Sniper Skin",     &g_skin_cfg.sniper_enabled);
            if (g_skin_cfg.sniper_enabled) {
                static int ni = 0;
                static const char* nn[SkinIds::SniperCount];
                static bool nb = false;
                if (!nb) { for (int i=0;i<SkinIds::SniperCount;i++) nn[i]=SkinIds::Sniper[i].name; nb=true; }
                if (ImGui::Combo("Sniper", &ni, nn, SkinIds::SniperCount))
                    g_skin_cfg.sniper_id = SkinIds::Sniper[ni].id;
            }

            ImGui::Checkbox("Heavy Skin",      &g_skin_cfg.heavy_enabled);
            if (g_skin_cfg.heavy_enabled) {
                static int hi = 0;
                static const char* hn[SkinIds::HeavyCount];
                static bool hb = false;
                if (!hb) { for (int i=0;i<SkinIds::HeavyCount;i++) hn[i]=SkinIds::Heavy[i].name; hb=true; }
                if (ImGui::Combo("Heavy", &hi, hn, SkinIds::HeavyCount))
                    g_skin_cfg.heavy_id = SkinIds::Heavy[hi].id;
            }

            ImGui::EndTabItem();
        }

        // ---- Info Tab ----
        if (ImGui::BeginTabItem("Info")) {
            ImGui::Text("Cryptonit Internal");
            ImGui::Text("Target: Standoff 2 v0.39.2");
            ImGui::Text("Players: %d", g_frame.player_count);
            if (g_frame.local.valid) {
                ImGui::Text("HP: %.0f / %.0f", g_frame.local.health,
                            g_frame.local.max_health);
                ImGui::Text("Pos: %.1f %.1f %.1f",
                            g_frame.local.position.x,
                            g_frame.local.position.y,
                            g_frame.local.position.z);
            }
            ImGui::Text("Menu: long-press Vol-Down");
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}

// ---- Frame render callback (called from eglSwapBuffers hook) ----
static void on_render(int sw, int sh) {
    // Update ESP screen positions this frame
    g_frame.screen_w = sw;
    g_frame.screen_h = sh;
    g_frame.crosshair = {sw * 0.5f, sh * 0.5f};

    // Precompute bone screen positions
    for (int i = 0; i < g_frame.player_count; i++) {
        auto& p = g_frame.players[i];
        world_to_screen(g_frame.view_proj, p.head_pos, p.screen_head, sw, sh);
        world_to_screen(g_frame.view_proj, p.feet_pos, p.screen_feet, sw, sh);
    }

    // Build draw context using ImGui draw list
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    DrawCtx draw;
    draw.sw = sw; draw.sh = sh;

    draw.line = [&](float x1, float y1, float x2, float y2,
                    float r, float g, float b, float a) {
        dl->AddLine({x1,y1}, {x2,y2},
                    IM_COL32((int)(r*255),(int)(g*255),(int)(b*255),(int)(a*255)));
    };
    draw.rect = [&](float x, float y, float w, float h,
                    float r, float g, float b, float a, bool filled) {
        ImU32 col = IM_COL32((int)(r*255),(int)(g*255),(int)(b*255),(int)(a*255));
        if (filled) dl->AddRectFilled({x,y},{x+w,y+h},col);
        else        dl->AddRect({x,y},{x+w,y+h},col);
    };
    draw.text = [&](float x, float y, const char* str,
                    float r, float g, float b, float a) {
        ImU32 col = IM_COL32((int)(r*255),(int)(g*255),(int)(b*255),(int)(a*255));
        // Shadow for readability
        dl->AddText({x+1.f,y+1.f}, IM_COL32(0,0,0,200), str);
        dl->AddText({x,y}, col, str);
    };
    draw.circle = [&](float cx, float cy, float rad,
                      float r, float g, float b, float a) {
        dl->AddCircle({cx,cy}, rad,
                      IM_COL32((int)(r*255),(int)(g*255),(int)(b*255),(int)(a*255)));
    };

    // FOV circle for aimbot
    if (g_aim_cfg.enabled) {
        dl->AddCircle({(float)sw*0.5f,(float)sh*0.5f}, g_aim_cfg.fov,
                      IM_COL32(255,255,255,60));
    }

    esp_render(g_frame, g_esp_cfg, draw);
    render_menu(sw, sh);
}

// ---- Game state reader thread ----
static void* reader_thread(void*) {
    auto& bridge = IL2CppBridge::get();
    while (!bridge.init()) {
        LOGI("IL2CPP not ready, retrying...");
        sleep(2);
    }
    LOGI("IL2CPP bridge initialized");

    // Resolve GameManager static instance
    void* gm_field = nullptr; // obtained via il2cpp bridge class cache
    // In practice: cache the GameManager static instance field pointer during init

    while (g_running) {
        usleep(8000); // ~120 Hz read loop

        // Read GameManager.Instance (static)
        void* gm_instance = nullptr;
        if (!gm_instance) continue;

        // Read local player and all players
        void* player_list = (void*)mem_read<uintptr_t>(
            (uintptr_t)gm_instance + Offsets::GameManager::player_list);
        if (!player_list) continue;

        // Read list backing array
        void* items = (void*)mem_read<uintptr_t>(
            (uintptr_t)player_list + Offsets::List::items);
        int   count = mem_read<int>(
            (uintptr_t)player_list + Offsets::List::size);
        if (!items || count <= 0 || count > 32) continue;

        void* local_raw = (void*)mem_read<uintptr_t>(
            (uintptr_t)gm_instance + Offsets::GameManager::local_player);

        g_frame.player_count = 0;

        for (int i = 0; i < count && i < 32; i++) {
            void* pc = (void*)mem_read<uintptr_t>(
                (uintptr_t)items + Offsets::Array::data + i * sizeof(uintptr_t));
            if (!pc) continue;

            PlayerSnapshot& snap = g_frame.players[g_frame.player_count];
            snap.addr     = (uintptr_t)pc;
            snap.valid    = true;
            snap.is_local = (pc == local_raw);
            snap.health   = mem_read<float>((uintptr_t)pc + Offsets::PlayerController::health);
            snap.max_health= mem_read<float>((uintptr_t)pc + Offsets::PlayerController::max_health);
            snap.team     = (Team)mem_read<int>((uintptr_t)pc + Offsets::PlayerController::team);
            snap.is_alive = snap.health > 0.f;
            snap.is_visible = true; // proper vis-check: Physics.Linecast in hook

            void* name_obj= (void*)mem_read<uintptr_t>(
                (uintptr_t)pc + Offsets::PlayerController::player_name);
            snap.name = bridge.read_string(name_obj);

            void* xform = (void*)mem_read<uintptr_t>(
                (uintptr_t)pc + Offsets::PlayerController::transform);
            snap.position = bridge.call_get_position(xform);

            void* anim = (void*)mem_read<uintptr_t>(
                (uintptr_t)pc + Offsets::PlayerController::animator);
            snap.head_pos = bridge.call_get_bone(anim, (int)BoneId::Head);
            snap.feet_pos = bridge.call_get_bone(anim, (int)BoneId::LeftFoot);
            for (int b = 0; b < (int)BoneId::Count; b++)
                snap.bones[b] = bridge.call_get_bone(anim, b);

            if (snap.is_local) g_frame.local = snap;

            snap.distance = snap.position.distance(g_frame.local.position);
            g_frame.player_count++;
        }

        // Read weapon state
        if (local_raw) {
            void* wc = (void*)mem_read<uintptr_t>(
                (uintptr_t)local_raw + Offsets::PlayerController::weapon_ctrl);
            if (wc) {
                g_frame.weapon.addr      = (uintptr_t)wc;
                g_frame.weapon.ammo_clip = mem_read<int>((uintptr_t)wc + Offsets::WeaponController::ammo_clip);
                g_frame.weapon.ammo_reserve= mem_read<int>((uintptr_t)wc + Offsets::WeaponController::ammo_reserve);
                g_frame.weapon.fire_rate = mem_read<float>((uintptr_t)wc + Offsets::WeaponController::fire_rate);
                g_frame.weapon.spread    = mem_read<float>((uintptr_t)wc + Offsets::WeaponController::spread);
                g_frame.weapon.type      = (WeaponType)mem_read<int>((uintptr_t)wc + Offsets::WeaponController::weapon_type);

                misc_apply_pre_frame(g_frame, g_misc_cfg, wc, local_raw);
                skin_changer_apply(g_skin_cfg, wc, local_raw);
            }
        }

        // Aimbot
        AimbotResult aim = aimbot_calc(g_frame, g_aim_cfg);
        aimbot_apply(aim, g_aim_cfg, nullptr, nullptr);
    }
    return nullptr;
}

// ---- Zygisk entry ----
// This function is called by the Zygisk module after app specialization
// See zygisk_module/module.cpp for the full Zygisk wrapper
extern "C" void cryptonit_main() {
    LOGI("Cryptonit loading...");

    uintptr_t il2cpp_base = get_module_base("libil2cpp.so");
    if (!il2cpp_base) {
        LOGE("libil2cpp.so not found in maps");
        return;
    }
    LOGI("libil2cpp.so base: 0x%lx", il2cpp_base);

    hooks_set_misc_config(&g_misc_cfg);
    if (!hooks_install(il2cpp_base))
        LOGE("Some hooks failed — partial functionality");

    overlay_init(on_render);

    pthread_t reader;
    pthread_create(&reader, nullptr, reader_thread, nullptr);
    pthread_detach(reader);

    LOGI("Cryptonit active");
}
