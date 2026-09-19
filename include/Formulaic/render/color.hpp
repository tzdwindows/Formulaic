#pragma once

#include <Formulaic/core/export.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace formulaic {

struct Color {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};

    constexpr Color() noexcept = default;
    constexpr Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) noexcept
        : r(red), g(green), b(blue), a(alpha) {}

    [[nodiscard]] static constexpr Color from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) noexcept {
        return Color(r, g, b, a);
    }

    [[nodiscard]] static constexpr Color from_rgba32(uint32_t val) noexcept {
        return Color(
            static_cast<uint8_t>((val >> 24) & 0xFF),
            static_cast<uint8_t>((val >> 16) & 0xFF),
            static_cast<uint8_t>((val >> 8) & 0xFF),
            static_cast<uint8_t>(val & 0xFF)
        );
    }

    [[nodiscard]] static constexpr Color from_bgra32(uint32_t val) noexcept {
        return Color(
            static_cast<uint8_t>((val >> 16) & 0xFF),
            static_cast<uint8_t>((val >> 8) & 0xFF),
            static_cast<uint8_t>(val & 0xFF),
            static_cast<uint8_t>((val >> 24) & 0xFF)
        );
    }

    [[nodiscard]] constexpr uint32_t to_rgba32() const noexcept {
        return (static_cast<uint32_t>(r) << 24) |
               (static_cast<uint32_t>(g) << 16) |
               (static_cast<uint32_t>(b) << 8)  |
               static_cast<uint32_t>(a);
    }

    [[nodiscard]] constexpr uint32_t to_bgra32() const noexcept {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8)  |
               static_cast<uint32_t>(b);
    }

    [[nodiscard]] constexpr Color with_alpha(uint8_t new_alpha) const noexcept {
        return Color(r, g, b, new_alpha);
    }

    constexpr bool operator==(const Color&) const noexcept = default;

    [[nodiscard]] static Color lerp(const Color& c1, const Color& c2, double t) noexcept {
        t = std::clamp(t, 0.0, 1.0);
        return Color(
            static_cast<uint8_t>(c1.r + (c2.r - c1.r) * t),
            static_cast<uint8_t>(c1.g + (c2.g - c1.g) * t),
            static_cast<uint8_t>(c1.b + (c2.b - c1.b) * t),
            static_cast<uint8_t>(c1.a + (c2.a - c1.a) * t)
        );
    }

    // Built-in palette presets
    static const Color Black;
    static const Color White;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Cyan;
    static const Color Magenta;
    static const Color Gray;
    static const Color DarkGray;
    static const Color LightGray;
    static const Color GridGray;
    static const Color AxisGray;
    static const Color BackgroundDark;
    static const Color NeonBlue;
    static const Color NeonGreen;
    static const Color NeonPink;
    static const Color Transparent;
};

inline constexpr Color Color::Black{0, 0, 0, 255};
inline constexpr Color Color::White{255, 255, 255, 255};
inline constexpr Color Color::Red{235, 60, 60, 255};
inline constexpr Color Color::Green{50, 205, 50, 255};
inline constexpr Color Color::Blue{65, 105, 225, 255};
inline constexpr Color Color::Yellow{255, 215, 0, 255};
inline constexpr Color Color::Cyan{0, 220, 220, 255};
inline constexpr Color Color::Magenta{220, 20, 180, 255};
inline constexpr Color Color::Gray{128, 128, 128, 255};
inline constexpr Color Color::DarkGray{50, 50, 55, 255};
inline constexpr Color Color::LightGray{200, 200, 205, 255};
inline constexpr Color Color::GridGray{45, 45, 55, 255};
inline constexpr Color AxisGrayFallback{160, 160, 175, 255};
inline constexpr Color Color::AxisGray{160, 160, 175, 255};
inline constexpr Color Color::BackgroundDark{18, 18, 24, 255};
inline constexpr Color Color::NeonBlue{0, 190, 255, 255};
inline constexpr Color Color::NeonGreen{57, 255, 20, 255};
inline constexpr Color Color::NeonPink{255, 20, 147, 255};
inline constexpr Color Color::Transparent{0, 0, 0, 0};

enum class ColormapType {
    Viridis,
    Plasma,
    Coolwarm,
    Jet,
    Grayscale
};

FORMULAIC_API Color sample_colormap(ColormapType type, double normalized_t) noexcept;

} // namespace formulaic
