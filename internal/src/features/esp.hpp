// language: C++17, file: internal/src/features/esp.hpp
#pragma once
#include "../../../shared/structs.hpp"
#include <functional>

struct ESPConfig {
    bool enabled       = true;
    bool boxes         = true;
    bool skeleton      = true;
    bool health_bar    = true;
    bool name_tag      = true;
    bool distance      = true;
    bool weapon_name   = true;
    bool snaplines     = false;
    bool radar         = true;
    bool visible_only  = false;  // show only visible enemies

    float box_r_enemy  = 1.0f, box_g_enemy  = 0.0f, box_b_enemy  = 0.0f;
    float box_r_ally   = 0.0f, box_g_ally   = 1.0f, box_b_ally   = 0.0f;
    float radar_range  = 100.0f; // meters
    float radar_size   = 150.0f; // pixels
    float radar_x      = 10.0f;
    float radar_y      = 10.0f;
};

// draw_line(x1,y1, x2,y2, r,g,b,a)
// draw_rect(x,y,w,h, r,g,b,a, filled)
// draw_text(x,y, str, r,g,b,a)
// draw_circle(cx,cy,r, r,g,b,a)
using DrawLineFn   = std::function<void(float,float,float,float, float,float,float,float)>;
using DrawRectFn   = std::function<void(float,float,float,float, float,float,float,float, bool)>;
using DrawTextFn   = std::function<void(float,float, const char*, float,float,float,float)>;
using DrawCircleFn = std::function<void(float,float,float, float,float,float,float)>;

struct DrawCtx {
    DrawLineFn   line;
    DrawRectFn   rect;
    DrawTextFn   text;
    DrawCircleFn circle;
    int sw, sh;
};

void esp_render(const FrameCtx& frame, const ESPConfig& cfg, const DrawCtx& draw);
