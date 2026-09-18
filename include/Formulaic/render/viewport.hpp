#pragma once

#include <Formulaic/core/export.hpp>
#include <Formulaic/core/types.hpp>
#include <algorithm>

namespace formulaic {

class FORMULAIC_API Viewport {
public:
    Viewport() = default;
    Viewport(int width, int height, Rect2D bounds = Rect2D(-10.0, 10.0, -10.0, 10.0))
        : width_(width), height_(height), bounds_(bounds) {}

    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] const Rect2D& bounds() const noexcept { return bounds_; }
    [[nodiscard]] Rect2D& bounds() noexcept { return bounds_; }

    void set_size(int w, int h) noexcept {
        width_ = std::max(1, w);
        height_ = std::max(1, h);
    }

    void set_bounds(const Rect2D& b) noexcept {
        bounds_ = b;
    }

    // World to Screen transformation (Screen origin top-left, Y downwards)
    [[nodiscard]] Point2I world_to_screen(const Point2D& world) const noexcept {
        if (width_ <= 0 || height_ <= 0) return {0, 0};
        const double w_span = bounds_.width();
        const double h_span = bounds_.height();
        if (w_span == 0.0 || h_span == 0.0) return {0, 0};

        const double sx = ((world.x - bounds_.x_min) / w_span) * (width_ - 1);
        const double sy = ((bounds_.y_max - world.y) / h_span) * (height_ - 1);

        return {static_cast<int>(std::round(sx)), static_cast<int>(std::round(sy))};
    }

    // World to Screen continuous (floating point)
    [[nodiscard]] Point2D world_to_screen_f(const Point2D& world) const noexcept {
        if (width_ <= 0 || height_ <= 0) return {0.0, 0.0};
        const double w_span = bounds_.width();
        const double h_span = bounds_.height();
        if (w_span == 0.0 || h_span == 0.0) return {0.0, 0.0};

        const double sx = ((world.x - bounds_.x_min) / w_span) * (width_ - 1);
        const double sy = ((bounds_.y_max - world.y) / h_span) * (height_ - 1);
        return {sx, sy};
    }

    // Screen to World transformation
    [[nodiscard]] Point2D screen_to_world(double sx, double sy) const noexcept {
        if (width_ <= 1 || height_ <= 1) return bounds_.center();
        const double wx = bounds_.x_min + (sx / (width_ - 1)) * bounds_.width();
        const double wy = bounds_.y_max - (sy / (height_ - 1)) * bounds_.height();
        return {wx, wy};
    }

    [[nodiscard]] Point2D screen_to_world(Point2I pt) const noexcept {
        return screen_to_world(static_cast<double>(pt.x), static_cast<double>(pt.y));
    }

    // Interactive operations
    void pan(double delta_screen_x, double delta_screen_y) noexcept {
        if (width_ <= 1 || height_ <= 1) return;
        const double dx_world = (delta_screen_x / (width_ - 1)) * bounds_.width();
        const double dy_world = (delta_screen_y / (height_ - 1)) * bounds_.height();
        bounds_.x_min -= dx_world;
        bounds_.x_max -= dx_world;
        bounds_.y_min += dy_world;
        bounds_.y_max += dy_world;
    }

    void zoom(double factor, Point2D focus_screen) noexcept {
        if (factor <= 0.0) return;
        const Point2D focus_world = screen_to_world(focus_screen.x, focus_screen.y);
        const double new_w = bounds_.width() / factor;
        const double new_h = bounds_.height() / factor;

        const double rx = (focus_world.x - bounds_.x_min) / bounds_.width();
        const double ry = (focus_world.y - bounds_.y_min) / bounds_.height();

        bounds_.x_min = focus_world.x - rx * new_w;
        bounds_.x_max = bounds_.x_min + new_w;
        bounds_.y_min = focus_world.y - ry * new_h;
        bounds_.y_max = bounds_.y_min + new_h;
    }

    void keep_aspect_ratio() noexcept {
        if (width_ <= 0 || height_ <= 0) return;
        const double screen_ratio = static_cast<double>(width_) / static_cast<double>(height_);
        const double current_w = bounds_.width();
        const double current_h = bounds_.height();
        const double current_ratio = current_w / current_h;

        const Point2D c = bounds_.center();
        if (current_ratio < screen_ratio) {
            const double new_w = current_h * screen_ratio;
            bounds_.x_min = c.x - new_w * 0.5;
            bounds_.x_max = c.x + new_w * 0.5;
        } else {
            const double new_h = current_w / screen_ratio;
            bounds_.y_min = c.y - new_h * 0.5;
            bounds_.y_max = c.y + new_h * 0.5;
        }
    }

private:
    int width_{800};
    int height_{600};
    Rect2D bounds_{-10.0, 10.0, -10.0, 10.0};
};

} // namespace formulaic
