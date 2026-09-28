// language: C++17, file: external/src/process.hpp, target: Android ARM64
// External memory reader — /proc/pid/mem approach, requires root
// Works without injection: runs as a separate root process alongside game
#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

struct MapRegion {
    uintptr_t start, end;
    bool readable, writable, executable;
    std::string name;
};

class ProcessMem {
public:
    explicit ProcessMem(int pid);
    ~ProcessMem();

    bool open();
    void close();
    bool is_open() const { return mem_fd_ >= 0; }

    bool read(uintptr_t addr, void* out, size_t size) const;
    bool write(uintptr_t addr, const void* in, size_t size);

    uintptr_t find_module(const char* name) const;
    std::vector<MapRegion> get_maps() const;

    template<typename T>
    T read(uintptr_t addr) const {
        T v{};
        read(addr, &v, sizeof(T));
        return v;
    }

    template<typename T>
    bool write(uintptr_t addr, const T& val) {
        return write(addr, &val, sizeof(T));
    }

    int pid() const { return pid_; }

private:
    int pid_;
    int mem_fd_ = -1;
};

// Find PID of a running package by name
int find_pid(const char* package_name);
