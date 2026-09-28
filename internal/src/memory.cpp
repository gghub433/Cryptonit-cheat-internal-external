// language: C++17, file: internal/src/memory.cpp, target: Android ARM64
// In-process memory R/W — already inside the game process via Zygisk injection
// No RPM/WPM needed; direct pointer dereference with nullptr guard
#include "memory.hpp"
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Cryptonit", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Cryptonit", __VA_ARGS__)

static bool is_mapped(uintptr_t addr, size_t size) {
    // Quick probe via mincore — page must be in the VMA
    unsigned char vec[1];
    return mincore(reinterpret_cast<void*>(addr & ~(getpagesize()-1)),
                   getpagesize(), vec) == 0;
}

bool mem_read(uintptr_t addr, void* out, size_t size) {
    if (!addr || !out || !size) return false;
    if (!is_mapped(addr, size)) return false;
    memcpy(out, reinterpret_cast<void*>(addr), size);
    return true;
}

bool mem_write(uintptr_t addr, const void* in, size_t size) {
    if (!addr || !in || !size) return false;
    // Make page writable if needed via mprotect
    uintptr_t page = addr & ~(uintptr_t)(getpagesize()-1);
    mprotect(reinterpret_cast<void*>(page), getpagesize(),
             PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy(reinterpret_cast<void*>(addr), in, size);
    return true;
}

// Patch a sequence of bytes; stores original for restore
bool mem_patch(uintptr_t addr, const uint8_t* patch, size_t len,
               uint8_t* orig_out) {
    if (orig_out) memcpy(orig_out, reinterpret_cast<void*>(addr), len);
    return mem_write(addr, patch, len);
}

// NOP out an ARM64 instruction (4 bytes = 0x1F200003)
bool mem_nop(uintptr_t addr) {
    static const uint8_t nop[4] = {0x1F, 0x20, 0x03, 0xD5};
    return mem_write(addr, nop, 4);
}

uintptr_t get_module_base(const char* name) {
    FILE* maps = fopen("/proc/self/maps", "r");
    if (!maps) return 0;
    char line[512];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, name)) {
            base = strtoull(line, nullptr, 16);
            break;
        }
    }
    fclose(maps);
    return base;
}
