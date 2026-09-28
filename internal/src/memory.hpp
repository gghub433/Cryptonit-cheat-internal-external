// language: C++17, file: internal/src/memory.hpp
#pragma once
#include <cstdint>
#include <cstddef>

bool     mem_read(uintptr_t addr, void* out, size_t size);
bool     mem_write(uintptr_t addr, const void* in, size_t size);
bool     mem_patch(uintptr_t addr, const uint8_t* patch, size_t len, uint8_t* orig_out = nullptr);
bool     mem_nop(uintptr_t addr);
uintptr_t get_module_base(const char* name);

template<typename T>
inline T mem_read(uintptr_t addr) {
    T v{};
    mem_read(addr, &v, sizeof(T));
    return v;
}

template<typename T>
inline bool mem_write(uintptr_t addr, const T& val) {
    return mem_write(addr, &val, sizeof(T));
}
