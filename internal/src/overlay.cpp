// language: C++17, file: internal/src/overlay.cpp, target: Android ARM64
// EGL hook overlay — intercepts eglSwapBuffers to draw ImGui on top of game
// Pattern: hook libEGL.so export -> init ImGui EGL backend -> render each frame
#include "overlay.hpp"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dobby.h>
#include <android/log.h>
#include <cstring>
#include <atomic>

// ImGui backend (EGL/GLES3 variant)
// Include paths depend on project setup — add imgui/ to include dirs
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "CryptonitOverlay"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)

static EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay, EGLSurface) = nullptr;
static std::atomic<bool> g_imgui_ready{false};
static int g_sw = 0, g_sh = 0;

// Callback set by main.cpp — called each frame to render cheat UI
static OverlayRenderFn g_render_fn = nullptr;

static void imgui_init(EGLDisplay dpy, EGLSurface surf) {
    EGLint w, h;
    eglQuerySurface(dpy, surf, EGL_WIDTH,  &w);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    g_sw = w; g_sh = h;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    // Custom Cryptonit color scheme
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding  = 6.f;
    style.FrameRounding   = 4.f;
    style.Colors[ImGuiCol_WindowBg]  = ImVec4(0.06f, 0.06f, 0.08f, 0.88f);
    style.Colors[ImGuiCol_TitleBg]   = ImVec4(0.10f, 0.00f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.00f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_Button]    = ImVec4(0.30f, 0.00f, 0.60f, 0.80f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.00f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.60f, 0.20f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.20f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_FrameBg]   = ImVec4(0.15f, 0.15f, 0.20f, 0.90f);
    style.Colors[ImGuiCol_Header]    = ImVec4(0.30f, 0.00f, 0.60f, 0.70f);

    ImGui_ImplAndroid_Init(nullptr); // Android input events handled separately
    ImGui_ImplOpenGL3_Init("#version 300 es");

    g_imgui_ready = true;
    LOGI("ImGui overlay initialized (%dx%d)", w, h);
}

static EGLBoolean hk_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    if (!g_imgui_ready.load(std::memory_order_relaxed))
        imgui_init(dpy, surf);

    if (g_imgui_ready && g_render_fn) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame();
        ImGui::NewFrame();

        g_render_fn(g_sw, g_sh);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return orig_eglSwapBuffers(dpy, surf);
}

bool overlay_init(OverlayRenderFn render_fn) {
    g_render_fn = render_fn;
    void* lib = dlopen("libEGL.so", RTLD_NOLOAD | RTLD_NOW);
    if (!lib) { LOGI("libEGL not loaded yet"); return false; }
    void* sym = dlsym(lib, "eglSwapBuffers");
    if (!sym) { LOGI("eglSwapBuffers not found"); return false; }
    int ret = DobbyHook(sym, (void*)hk_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
    LOGI("eglSwapBuffers hook: %s", ret == 0 ? "OK" : "FAIL");
    return ret == 0;
}

void overlay_shutdown() {
    if (!g_imgui_ready) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
    g_imgui_ready = false;
    if (orig_eglSwapBuffers)
        DobbyDestroy(reinterpret_cast<void*>(orig_eglSwapBuffers));
}

int overlay_get_width()  { return g_sw; }
int overlay_get_height() { return g_sh; }
