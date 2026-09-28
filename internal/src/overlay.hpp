// language: C++17, file: internal/src/overlay.hpp
#pragma once
#include <functional>

// Called every frame with screen w/h — draw ImGui windows and ESP here
using OverlayRenderFn = std::function<void(int sw, int sh)>;

bool overlay_init(OverlayRenderFn render_fn);
void overlay_shutdown();
int  overlay_get_width();
int  overlay_get_height();
