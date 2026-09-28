// language: C++17, file: internal/src/features/esp.cpp, target: Android ARM64
// ESP rendering: boxes, skeleton, health bars, radar, names, distance
#include "esp.hpp"
#include <cstdio>
#include <cmath>
#include <algorithm>

static const int SKELETON_LINES[][2] = {
    {(int)BoneId::Head,       (int)BoneId::Neck},
    {(int)BoneId::Neck,       (int)BoneId::Spine1},
    {(int)BoneId::Spine1,     (int)BoneId::Spine},
    {(int)BoneId::Spine,      (int)BoneId::Hips},
    {(int)BoneId::Hips,       (int)BoneId::LeftUpLeg},
    {(int)BoneId::Hips,       (int)BoneId::RightUpLeg},
    {(int)BoneId::LeftUpLeg,  (int)BoneId::LeftLeg},
    {(int)BoneId::LeftLeg,    (int)BoneId::LeftFoot},
    {(int)BoneId::RightUpLeg, (int)BoneId::RightLeg},
    {(int)BoneId::RightLeg,   (int)BoneId::RightFoot},
    {(int)BoneId::Spine1,     (int)BoneId::LeftArm},
    {(int)BoneId::LeftArm,    (int)BoneId::LeftForeArm},
    {(int)BoneId::LeftForeArm,(int)BoneId::LeftHand},
    {(int)BoneId::Spine1,     (int)BoneId::RightArm},
    {(int)BoneId::RightArm,   (int)BoneId::RightForeArm},
    {(int)BoneId::RightForeArm,(int)BoneId::RightHand},
};

static const char* weapon_name(WeaponType t) {
    switch (t) {
        case WeaponType::Knife:   return "Knife";
        case WeaponType::Pistol:  return "Pistol";
        case WeaponType::Rifle:   return "Rifle";
        case WeaponType::Shotgun: return "Shotgun";
        case WeaponType::Sniper:  return "Sniper";
        case WeaponType::SMG:     return "SMG";
        case WeaponType::Grenade: return "Grenade";
        default:                  return "Unknown";
    }
}

static void draw_player(const PlayerSnapshot& p, const ESPConfig& cfg,
                        const DrawCtx& draw, const FrameCtx& frame) {
    if (!p.valid || !p.is_alive) return;
    if (cfg.visible_only && !p.is_visible) return;

    // Color selection — enemy red, ally green
    bool enemy = (p.team != frame.local.team);
    float r = enemy ? cfg.box_r_enemy : cfg.box_r_ally;
    float g = enemy ? cfg.box_g_enemy : cfg.box_g_ally;
    float b = enemy ? cfg.box_b_enemy : cfg.box_b_ally;
    float a = 1.0f;

    Vector2 head = p.screen_head;
    Vector2 feet = p.screen_feet;

    // 2D bounding box from head/feet screen positions
    if (cfg.boxes && head.y < feet.y) {
        float height = feet.y - head.y;
        float width  = height * 0.4f;  // rough aspect for humanoid
        float x = head.x - width * 0.5f;
        float y = head.y;

        // Outline (black) for readability
        draw.rect(x-1, y-1, width+2, height+2, 0,0,0, a, false);
        draw.rect(x,   y,   width,   height,   r,g,b, a, false);

        if (cfg.health_bar && p.max_health > 0.f) {
            float hp_ratio  = p.health / p.max_health;
            float hp_height = height * hp_ratio;
            float hbx = x - 6.f;
            // background
            draw.rect(hbx, y, 4.f, height, 0.15f,0.15f,0.15f, 0.8f, true);
            // foreground — green to yellow to red
            float hr = 1.f - hp_ratio;
            float hg = hp_ratio;
            draw.rect(hbx, y + height - hp_height, 4.f, hp_height, hr, hg, 0.f, 1.f, true);
        }

        if (cfg.name_tag || cfg.distance || cfg.weapon_name) {
            char buf[128];
            int len = 0;
            if (cfg.name_tag && !p.name.empty())
                len += snprintf(buf+len, sizeof(buf)-len, "%s", p.name.c_str());
            if (cfg.distance)
                len += snprintf(buf+len, sizeof(buf)-len,
                                len ? " [%.0fm]" : "%.0fm", p.distance);
            if (cfg.weapon_name)
                len += snprintf(buf+len, sizeof(buf)-len,
                                len ? " | %s" : "%s", weapon_name(p.weapon));
            draw.text(x, y - 14.f, buf, r, g, b, a);
        }
    }

    // Skeleton
    if (cfg.skeleton) {
        for (auto& link : SKELETON_LINES) {
            const Vector3& a3 = p.bones[link[0]];
            const Vector3& b3 = p.bones[link[1]];
            Vector2 sa, sb;
            if (world_to_screen(frame.view_proj, a3, sa, draw.sw, draw.sh) &&
                world_to_screen(frame.view_proj, b3, sb, draw.sw, draw.sh)) {
                draw.line(sa.x, sa.y, sb.x, sb.y, r, g, b, 0.85f);
            }
        }
    }

    // Snapline — from screen bottom center to feet
    if (cfg.snaplines) {
        float cx = draw.sw * 0.5f;
        float cy = (float)draw.sh;
        draw.line(cx, cy, feet.x, feet.y, r, g, b, 0.5f);
    }
}

static void draw_radar(const FrameCtx& frame, const ESPConfig& cfg,
                       const DrawCtx& draw) {
    if (!cfg.radar) return;
    float rx = cfg.radar_x, ry = cfg.radar_y;
    float rs = cfg.radar_size;
    float half = rs * 0.5f;

    // Background
    draw.rect(rx, ry, rs, rs, 0.f, 0.f, 0.f, 0.6f, true);
    draw.rect(rx, ry, rs, rs, 0.5f, 0.5f, 0.5f, 1.f, false);

    // Cross-hair center marker
    draw.line(rx+half-3, ry+half, rx+half+3, ry+half, 1,1,1,0.8f);
    draw.line(rx+half, ry+half-3, rx+half, ry+half+3, 1,1,1,0.8f);

    const Vector3& lp = frame.local.position;

    for (int i = 0; i < frame.player_count; i++) {
        const PlayerSnapshot& p = frame.players[i];
        if (!p.valid || !p.is_alive || p.is_local) continue;

        float dx = p.position.x - lp.x;
        float dz = p.position.z - lp.z;
        float dist = std::sqrt(dx*dx + dz*dz);
        if (dist > cfg.radar_range) continue;

        float scale = (half / cfg.radar_range);
        float sx = rx + half + dx * scale;
        float sy = ry + half - dz * scale; // Z is forward in Unity

        bool enemy = (p.team != frame.local.team);
        float cr = enemy ? 1.f : 0.f;
        float cg = enemy ? 0.f : 1.f;

        draw.rect(sx-2.5f, sy-2.5f, 5.f, 5.f, cr, cg, 0.f, 1.f, true);
    }
}

void esp_render(const FrameCtx& frame, const ESPConfig& cfg, const DrawCtx& draw) {
    if (!cfg.enabled) return;
    draw_radar(frame, cfg, draw);
    for (int i = 0; i < frame.player_count; i++)
        draw_player(frame.players[i], cfg, draw, frame);
}
