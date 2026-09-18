#pragma once

#include <Formulaic/core/export.hpp>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace formulaic {

struct Point2D {
    double x{0.0};
    double y{0.0};

    constexpr Point2D() noexcept = default;
    constexpr Point2D(double in_x, double in_y) noexcept : x(in_x), y(in_y) {}

    [[nodiscard]] constexpr Point2D operator+(const Point2D& other) const noexcept {
        return {x + other.x, y + other.y};
    }
    [[nodiscard]] constexpr Point2D operator-(const Point2D& other) const noexcept {
        return {x - other.x, y - other.y};
    }
    [[nodiscard]] constexpr Point2D operator*(double scalar) const noexcept {
        return {x * scalar, y * scalar};
    }
    [[nodiscard]] constexpr Point2D operator/(double scalar) const noexcept {
        return {x / scalar, y / scalar};
    }
};

struct Point2I {
    int x{0};
    int y{0};

    constexpr Point2I() noexcept = default;
    constexpr Point2I(int in_x, int in_y) noexcept : x(in_x), y(in_y) {}

    [[nodiscard]] constexpr bool operator==(const Point2I& other) const noexcept {
        return x == other.x && y == other.y;
    }
};

struct Size2I {
    int width{0};
    int height{0};

    constexpr Size2I() noexcept = default;
    constexpr Size2I(int w, int h) noexcept : width(w), height(h) {}

    [[nodiscard]] constexpr int area() const noexcept { return width * height; }
};

struct Rect2D {
    double x_min{-10.0};
    double x_max{10.0};
    double y_min{-10.0};
    double y_max{10.0};

    constexpr Rect2D() noexcept = default;
    constexpr Rect2D(double x0, double x1, double y0, double y1) noexcept
        : x_min(x0), x_max(x1), y_min(y0), y_max(y1) {}

    [[nodiscard]] constexpr double width() const noexcept { return x_max - x_min; }
    [[nodiscard]] constexpr double height() const noexcept { return y_max - y_min; }
    [[nodiscard]] constexpr Point2D center() const noexcept {
        return {(x_min + x_max) * 0.5, (y_min + y_max) * 0.5};
    }
    [[nodiscard]] constexpr bool contains(const Point2D& pt) const noexcept {
        return pt.x >= x_min && pt.x <= x_max && pt.y >= y_min && pt.y <= y_max;
    }
};

enum class BlendMode : uint8_t {
    Replace,
    AlphaBlend,
    Additive,
    Multiply
};

enum class MouseButton : uint8_t {
    None = 0,
    Left,
    Right,
    Middle
};

enum class MouseEventType : uint8_t {
    Move,
    Down,
    Up,
    Wheel,
    Enter,
    Leave
};

struct MouseEvent {
    MouseEventType type{MouseEventType::Move};
    MouseButton button{MouseButton::None};
    Point2I screen_pos{0, 0};
    Point2D world_pos{0.0, 0.0};
    double wheel_delta{0.0};
    bool ctrl_down{false};
    bool shift_down{false};
    bool alt_down{false};
};

struct HoverInfo {
    bool is_hovered{false};
    std::string target_name;
    Point2D world_pos{0.0, 0.0};
    Point2I screen_pos{0, 0};
    double value{0.0};
    double distance_px{0.0};
    std::string detail_text;
};

} // namespace formulaic
