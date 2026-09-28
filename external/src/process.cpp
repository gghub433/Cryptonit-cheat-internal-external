// language: C++17, file: external/src/process.cpp, target: Android ARM64
// /proc/pid/mem reader with pread64 for cross-process memory access (root required)
#include "process.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/uio.h>  // process_vm_readv (alternative path)
#include <android/log.h>

#define TAG "CryptonitExt"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

ProcessMem::ProcessMem(int pid) : pid_(pid) {}

ProcessMem::~ProcessMem() { close(); }

bool ProcessMem::open() {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/mem", pid_);
    mem_fd_ = ::open(path, O_RDWR);
    if (mem_fd_ < 0) {
        // Try read-only if write not needed
        mem_fd_ = ::open(path, O_RDONLY);
    }
    if (mem_fd_ < 0) {
        LOGE("Cannot open %s: %s", path, strerror(errno));
        return false;
    }
    LOGI("Opened /proc/%d/mem fd=%d", pid_, mem_fd_);
    return true;
}

void ProcessMem::close() {
    if (mem_fd_ >= 0) { ::close(mem_fd_); mem_fd_ = -1; }
}

bool ProcessMem::read(uintptr_t addr, void* out, size_t size) const {
    if (mem_fd_ < 0 || !out || !size) return false;
    ssize_t n = pread64(mem_fd_, out, size, (off64_t)addr);
    return n == (ssize_t)size;
}

bool ProcessMem::write(uintptr_t addr, const void* in, size_t size) {
    if (mem_fd_ < 0 || !in || !size) return false;
    ssize_t n = pwrite64(mem_fd_, in, size, (off64_t)addr);
    return n == (ssize_t)size;
}

uintptr_t ProcessMem::find_module(const char* name) const {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid_);
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    char line[512];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, name)) {
            base = strtoull(line, nullptr, 16);
            break;
        }
    }
    fclose(f);
    return base;
}

std::vector<MapRegion> ProcessMem::get_maps() const {
    std::vector<MapRegion> regions;
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid_);
    FILE* f = fopen(path, "r");
    if (!f) return regions;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        MapRegion r{};
        char perms[5], pathname[256] = {};
        unsigned long long start, end;
        sscanf(line, "%llx-%llx %4s %*s %*s %*s %255s",
               &start, &end, perms, pathname);
        r.start      = (uintptr_t)start;
        r.end        = (uintptr_t)end;
        r.readable   = perms[0] == 'r';
        r.writable   = perms[1] == 'w';
        r.executable = perms[2] == 'x';
        r.name       = pathname;
        if (r.readable) regions.push_back(r);
    }
    fclose(f);
    return regions;
}

// Scan /proc to find PID for a given package
int find_pid(const char* package_name) {
    DIR* proc = opendir("/proc");
    if (!proc) return -1;
    dirent* ent;
    while ((ent = readdir(proc))) {
        if (ent->d_type != DT_DIR) continue;
        char* end;
        long pid = strtol(ent->d_name, &end, 10);
        if (*end) continue;
        char cmd_path[64];
        snprintf(cmd_path, sizeof(cmd_path), "/proc/%ld/cmdline", pid);
        FILE* f = fopen(cmd_path, "r");
        if (!f) continue;
        char cmdline[256];
        size_t n = fread(cmdline, 1, sizeof(cmdline)-1, f);
        fclose(f);
        cmdline[n] = '\0';
        if (strstr(cmdline, package_name)) {
            closedir(proc);
            return (int)pid;
        }
    }
    closedir(proc);
    return -1;
}
