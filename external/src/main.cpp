// language: C++17, file: external/src/main.cpp, target: Android ARM64
// Cryptonit External — standalone root process, reads game memory via /proc/pid/mem
// Renders overlay via Android SurfaceView overlay permission (SYSTEM_ALERT_WINDOW)
// Build as a standalone executable, launch with: su -c /data/local/tmp/cryptonit_ext
#include "process.hpp"
#include "../../shared/structs.hpp"
#include "../../shared/offsets.hpp"
#include "../../shared/math.hpp"
#include <android/log.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <thread>

// For overlay rendering we use a JNI bridge to launch an Android Service
// that draws on a TYPE_APPLICATION_OVERLAY window. This is the standard
// external cheat approach on Android (doesn't require injection).
// The overlay service code is in external/java/ (separate Android project).
// Here we implement the native side that the Java service calls via JNI.

#define TAG "CryptonitExt"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static ProcessMem* g_proc = nullptr;
static FrameCtx    g_frame{};
static std::atomic<bool> g_running{true};

// ---- Memory helpers bound to the target process ----
template<typename T>
static T read_game(uintptr_t addr) {
    return g_proc ? g_proc->read<T>(addr) : T{};
}

// ---- Resolve GameManager static instance ----
// External approach: scan for il2cpp metadata sections to find static field offsets
// For a known version, the static instance lives at a known offset from libil2cpp base
static uintptr_t resolve_game_manager(uintptr_t il2cpp_base) {
    // GameManager static class data segment offset (derived by Ghidra RE)
    // il2cpp statics live in .bss section at predictable RVA per version
    constexpr uintptr_t gm_statics_rva = 0x01C8A000; // v1.37.x — re-derive per patch
    uintptr_t statics_addr = il2cpp_base + gm_statics_rva;
    return read_game<uintptr_t>(statics_addr + Offsets::GameManager::instance);
}

// ---- Main reader loop ----
static void reader_loop() {
    LOGI("Reader loop starting...");
    while (g_running && g_proc && g_proc->is_open()) {
        usleep(8000); // ~120 Hz

        uintptr_t il2cpp_base = g_proc->find_module("libil2cpp.so");
        if (!il2cpp_base) { sleep(1); continue; }

        uintptr_t gm = resolve_game_manager(il2cpp_base);
        if (!gm) continue;

        bool active = read_game<bool>(gm + Offsets::GameManager::is_game_active);
        if (!active) continue;

        uintptr_t player_list = read_game<uintptr_t>(gm + Offsets::GameManager::player_list);
        uintptr_t local_player= read_game<uintptr_t>(gm + Offsets::GameManager::local_player);
        if (!player_list) continue;

        uintptr_t items = read_game<uintptr_t>(player_list + Offsets::List::items);
        int count       = read_game<int>(player_list + Offsets::List::size);
        if (!items || count <= 0 || count > 32) continue;

        g_frame.player_count = 0;

        for (int i = 0; i < count && i < 32; i++) {
            uintptr_t pc = read_game<uintptr_t>(
                items + Offsets::Array::data + i * sizeof(uintptr_t));
            if (!pc) continue;

            PlayerSnapshot& snap = g_frame.players[g_frame.player_count];
            snap.addr      = pc;
            snap.valid     = true;
            snap.is_local  = (pc == local_player);
            snap.health    = read_game<float>(pc + Offsets::PlayerController::health);
            snap.max_health= read_game<float>(pc + Offsets::PlayerController::max_health);
            snap.team      = (Team)read_game<int>(pc + Offsets::PlayerController::team);
            snap.is_alive  = snap.health > 0.f;

            // Position via Transform offset chain
            uintptr_t xform = read_game<uintptr_t>(pc + Offsets::PlayerController::transform);
            if (xform) {
                // Unity Transform stores native TransformInternal*; local position at +0x90
                // exact offset derived by RE — read position directly from native transform
                constexpr uintptr_t native_pos_off = 0x90;
                uintptr_t native = read_game<uintptr_t>(xform + 0x10);
                if (native)
                    snap.position = read_game<Vector3>(native + native_pos_off);
            }

            // Head bone — Animator.GetBoneTransform can't be called externally
            // Approximate head position: position + (0, 1.7, 0)
            snap.head_pos = snap.position + Vector3{0.f, 1.7f, 0.f};
            snap.feet_pos = snap.position;

            if (snap.is_local) {
                g_frame.local = snap;
            }
            snap.distance = snap.position.distance(g_frame.local.position);
            g_frame.player_count++;
        }

        // Camera — find main camera object
        // External: we read the ViewProjectionMatrix directly from camera memory
        // Camera.projectionMatrix and worldToCameraMatrix are stored in Camera component
        // For simplicity, we derive VP from field-of-view + device orientation each frame
        // Full implementation would RE the camera object offset from GameManager
    }
    LOGI("Reader loop exited");
}

// ---- JNI export: called from overlay Java service to get frame data ----
// Returns serialized player array for drawing on the Java canvas
extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_cryptonit_ext_OverlayService_nativeGetFrame(JNIEnv* env, jobject) {
    // Serialize g_frame.players into a compact byte array for Java-side drawing
    int count = g_frame.player_count;
    int stride = sizeof(float)*6 + sizeof(int)*2; // x,y (head screen), x,y (feet), dist, health, team, alive
    jbyteArray arr = env->NewByteArray(4 + count * stride);
    if (!arr) return nullptr;
    std::vector<uint8_t> buf(4 + count * stride);
    memcpy(buf.data(), &count, 4);
    for (int i = 0; i < count; i++) {
        const auto& p = g_frame.players[i];
        float vals[6] = {
            p.screen_head.x, p.screen_head.y,
            p.screen_feet.x, p.screen_feet.y,
            p.distance, p.health
        };
        int info[2] = {(int)p.team, (int)p.is_alive};
        uint8_t* dst = buf.data() + 4 + i*stride;
        memcpy(dst,        vals,  sizeof(vals));
        memcpy(dst+sizeof(vals), info, sizeof(info));
    }
    env->SetByteArrayRegion(arr, 0, buf.size(),
                            reinterpret_cast<const jbyte*>(buf.data()));
    return arr;
}

extern "C" JNIEXPORT void JNICALL
Java_com_cryptonit_ext_OverlayService_nativeInit(JNIEnv*, jobject) {
    int pid = find_pid("com.axlebolt.standoff2");
    if (pid < 0) { LOGE("Standoff 2 not running"); return; }
    LOGI("Found Standoff 2 PID: %d", pid);
    g_proc = new ProcessMem(pid);
    if (!g_proc->open()) { delete g_proc; g_proc = nullptr; return; }
    g_running = true;
    std::thread(reader_loop).detach();
}

extern "C" JNIEXPORT void JNICALL
Java_com_cryptonit_ext_OverlayService_nativeStop(JNIEnv*, jobject) {
    g_running = false;
    if (g_proc) { g_proc->close(); delete g_proc; g_proc = nullptr; }
}

// ---- Standalone mode (run directly as root binary) ----
int main(int argc, char** argv) {
    LOGI("Cryptonit External starting...");

    int pid = find_pid("com.axlebolt.standoff2");
    while (pid < 0) {
        printf("Waiting for Standoff 2...\n");
        sleep(2);
        pid = find_pid("com.axlebolt.standoff2");
    }
    printf("Standoff 2 PID: %d\n", pid);

    ProcessMem proc(pid);
    if (!proc.open()) {
        printf("Cannot open /proc/%d/mem (need root)\n", pid);
        return 1;
    }
    g_proc = &proc;
    g_running = true;

    // Headless mode — dump player data to stdout for debugging
    // In overlay mode, this is replaced by the JNI service above
    int ticks = 0;
    while (g_running) {
        usleep(500000);
        uintptr_t il2cpp_base = proc.find_module("libil2cpp.so");
        if (!il2cpp_base) { printf("libil2cpp not mapped yet\n"); continue; }
        printf("[%d] il2cpp base: 0x%lx | players: %d\n",
               ticks++, il2cpp_base, g_frame.player_count);
        for (int i = 0; i < g_frame.player_count; i++) {
            const auto& p = g_frame.players[i];
            printf("  [%d] %s team=%d hp=%.0f dist=%.1fm pos=(%.1f,%.1f,%.1f)\n",
                   i, p.name.c_str(), (int)p.team, p.health, p.distance,
                   p.position.x, p.position.y, p.position.z);
        }
    }
    return 0;
}
