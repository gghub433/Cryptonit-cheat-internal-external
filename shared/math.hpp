// language: C++17, file: shared/math.hpp, target: Android ARM64
// Vector/Matrix math — Unity coordinate system (left-handed, Y-up)
#pragma once
#include <cmath>
#include <algorithm>

struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
    Vector2 operator+(const Vector2& o) const { return {x+o.x, y+o.y}; }
    Vector2 operator-(const Vector2& o) const { return {x-o.x, y-o.y}; }
    Vector2 operator*(float s) const { return {x*s, y*s}; }
    float length() const { return std::sqrt(x*x + y*y); }
};

struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3 operator+(const Vector3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vector3 operator*(float s) const { return {x*s, y*s, z*s}; }
    float dot(const Vector3& o) const { return x*o.x + y*o.y + z*o.z; }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vector3 normalized() const {
        float l = length();
        return l > 0.0001f ? Vector3{x/l, y/l, z/l} : Vector3{};
    }
    float distance(const Vector3& o) const { return (*this - o).length(); }
};

struct Vector4 {
    float x, y, z, w;
};

// Column-major 4x4, same layout Unity uses
struct Matrix4x4 {
    float m[4][4];

    Vector3 transform_point(const Vector3& p) const {
        float x = m[0][0]*p.x + m[1][0]*p.y + m[2][0]*p.z + m[3][0];
        float y = m[0][1]*p.x + m[1][1]*p.y + m[2][1]*p.z + m[3][1];
        float z = m[0][2]*p.x + m[1][2]*p.y + m[2][2]*p.z + m[3][2];
        float w = m[0][3]*p.x + m[1][3]*p.y + m[2][3]*p.z + m[3][3];
        if (std::abs(w) < 1e-6f) return {};
        return {x/w, y/w, z/w};
    }
};

// World-to-screen using Unity camera matrix
// Returns false if point is behind camera
inline bool world_to_screen(const Matrix4x4& vp, const Vector3& world,
                             Vector2& screen, int sw, int sh) {
    Vector3 clip = vp.transform_point(world);
    if (clip.z < 0.01f) return false;
    screen.x = (clip.x * 0.5f + 0.5f) * sw;
    screen.y = (1.0f - (clip.y * 0.5f + 0.5f)) * sh;
    return true;
}

inline float calc_fov(const Vector2& center, const Vector2& target) {
    float dx = target.x - center.x;
    float dy = target.y - center.y;
    return std::sqrt(dx*dx + dy*dy);
}

// Smooth aim interpolation
inline Vector2 smooth_to(const Vector2& current, const Vector2& target, float smooth) {
    return current + (target - current) * (1.0f / std::max(smooth, 1.0f));
}
