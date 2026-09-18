#pragma once

#include <Formulaic/core/export.hpp>
#include <Formulaic/core/types.hpp>
#include <Formulaic/render/color.hpp>
#include <memory>
#include <string_view>
#include <vector>

namespace formulaic {

class FORMULAIC_API FrameBuffer {
public:
    FrameBuffer() = default;
    FrameBuffer(int width, int height, Color clear_color = Color::BackgroundDark);

    void resize(int width, int height, Color clear_color = Color::BackgroundDark);

    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] size_t pixel_count() const noexcept { return static_cast<size_t>(width_) * height_; }
    [[nodiscard]] size_t byte_size() const noexcept { return pixel_count() * sizeof(uint32_t); }
    [[nodiscard]] size_t stride_bytes() const noexcept { return static_cast<size_t>(width_) * sizeof(uint32_t); }

    [[nodiscard]] const uint32_t* data() const noexcept { return pixels_.data(); }
    [[nodiscard]] uint32_t* data() noexcept { return pixels_.data(); }

    [[nodiscard]] const uint8_t* raw_bytes() const noexcept {
        return reinterpret_cast<const uint8_t*>(pixels_.data());
    }

    void clear(Color color) noexcept;

    inline void set_pixel(int x, int y, Color color) noexcept {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            // Store as BGRA (little-endian: B, G, R, A), perfectly matching Windows DIBs
            pixels_[static_cast<size_t>(y) * width_ + x] = color.to_bgra32();
        }
    }

    [[nodiscard]] inline Color get_pixel(int x, int y) const noexcept {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            return Color::from_bgra32(pixels_[static_cast<size_t>(y) * width_ + x]);
        }
        return Color::Transparent;
    }

    void blend_pixel(int x, int y, Color color, BlendMode mode = BlendMode::AlphaBlend) noexcept;

    // Primitives
    void draw_line(int x0, int y0, int x1, int y1, Color color, int thickness = 1) noexcept;
    void draw_line_aa(double x0, double y0, double x1, double y1, Color color) noexcept;
    void draw_rect(int x, int y, int w, int h, Color color) noexcept;
    void fill_rect(int x, int y, int w, int h, Color color) noexcept;
    void draw_circle(int cx, int cy, int radius, Color color) noexcept;
    void fill_circle(int cx, int cy, int radius, Color color) noexcept;
    void draw_text(int x, int y, std::string_view text, Color color, int scale = 1) noexcept;

    // Copy utilities
    void copy_to_rgba(std::vector<uint8_t>& out_rgba) const;

private:
    int width_{0};
    int height_{0};
    std::vector<uint32_t> pixels_; // Stored as 32-bit BGRA (0xAARRGGBB in little endian: BB, GG, RR, AA)
};

} // namespace formulaic
