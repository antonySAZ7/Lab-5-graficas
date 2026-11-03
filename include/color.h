#pragma once

#include <algorithm>
#include <cstdint>

#include <glm/glm.hpp>

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    constexpr Color() = default;
    constexpr Color(std::uint8_t red, std::uint8_t green, std::uint8_t blue,
                    std::uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}

    constexpr std::uint32_t toARGB() const {
        return (static_cast<std::uint32_t>(a) << 24U) |
               (static_cast<std::uint32_t>(r) << 16U) |
               (static_cast<std::uint32_t>(g) << 8U) |
               static_cast<std::uint32_t>(b);
    }
};

inline Color toColor(const glm::vec3& linearColor) {
    const glm::vec3 value = glm::clamp(linearColor, 0.0f, 1.0f) * 255.0f;
    return Color(static_cast<std::uint8_t>(value.r),
                 static_cast<std::uint8_t>(value.g),
                 static_cast<std::uint8_t>(value.b));
}

