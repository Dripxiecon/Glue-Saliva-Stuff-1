#pragma once

#include <cmath>

namespace gss::math {

struct Vec3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    constexpr Vec3 operator+(const Vec3& rhs) const noexcept { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    constexpr Vec3 operator-(const Vec3& rhs) const noexcept { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    constexpr Vec3 operator*(float scalar) const noexcept { return {x * scalar, y * scalar, z * scalar}; }
    constexpr Vec3 operator/(float scalar) const noexcept { return {x / scalar, y / scalar, z / scalar}; }
    constexpr float dot(const Vec3& rhs) const noexcept { return x * rhs.x + y * rhs.y + z * rhs.z; }
    constexpr Vec3 cross(const Vec3& rhs) const noexcept {
        return {y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x};
    }
    float length() const noexcept { return std::sqrt(dot(*this)); }
    Vec3 normalized() const noexcept {
        const float magnitude = length();
        return magnitude > 1.0e-8f ? *this / magnitude : Vec3{};
    }
};

} // namespace gss::math
