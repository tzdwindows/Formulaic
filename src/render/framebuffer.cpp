#include <Formulaic/render/framebuffer.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace formulaic {

namespace {

// Compact 5x7 ASCII bitmap font (characters 32 to 126)
// Each character is 5 columns, 7 bits high
static const uint8_t kFont5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 's'
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 '}'
    {0x10, 0x08, 0x08, 0x10, 0x08}  // 126 '~'
};

} // anonymous namespace

FrameBuffer::FrameBuffer(int width, int height, Color clear_color) {
    resize(width, height, clear_color);
}

void FrameBuffer::resize(int width, int height, Color clear_color) {
    width_ = std::max(1, width);
    height_ = std::max(1, height);
    pixels_.assign(pixel_count(), clear_color.to_bgra32());
}

void FrameBuffer::clear(Color color) noexcept {
    const uint32_t val = color.to_bgra32();
    std::fill(pixels_.begin(), pixels_.end(), val);
}

void FrameBuffer::blend_pixel(int x, int y, Color color, BlendMode mode) noexcept {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;

    const size_t idx = static_cast<size_t>(y) * width_ + x;
    if (mode == BlendMode::Replace || color.a == 255) {
        pixels_[idx] = color.to_bgra32();
        return;
    }
    if (color.a == 0) return;

    Color dst = Color::from_bgra32(pixels_[idx]);
    if (mode == BlendMode::AlphaBlend) {
        const float alpha = color.a / 255.0f;
        const float inv_alpha = 1.0f - alpha;
        dst.r = static_cast<uint8_t>(color.r * alpha + dst.r * inv_alpha);
        dst.g = static_cast<uint8_t>(color.g * alpha + dst.g * inv_alpha);
        dst.b = static_cast<uint8_t>(color.b * alpha + dst.b * inv_alpha);
        dst.a = std::max(dst.a, color.a);
    } else if (mode == BlendMode::Additive) {
        dst.r = static_cast<uint8_t>(std::min(255, dst.r + (color.r * color.a) / 255));
        dst.g = static_cast<uint8_t>(std::min(255, dst.g + (color.g * color.a) / 255));
        dst.b = static_cast<uint8_t>(std::min(255, dst.b + (color.b * color.a) / 255));
    } else if (mode == BlendMode::Multiply) {
        dst.r = static_cast<uint8_t>((dst.r * color.r) / 255);
        dst.g = static_cast<uint8_t>((dst.g * color.g) / 255);
        dst.b = static_cast<uint8_t>((dst.b * color.b) / 255);
    }
    pixels_[idx] = dst.to_bgra32();
}

void FrameBuffer::draw_line(int x0, int y0, int x1, int y1, Color color, int thickness) noexcept {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (thickness <= 1) {
            set_pixel(x0, y0, color);
        } else {
            fill_circle(x0, y0, thickness / 2, color);
        }

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Distance-Field Sub-pixel Anti-Aliased Line Drawing with Continuous Thickness
void FrameBuffer::draw_line_aa(double x0, double y0, double x1, double y1, Color color, double thickness) noexcept {
    if (thickness <= 0.0 || color.a == 0) return;

    const double radius = thickness * 0.5;
    const double radius_plus_1 = radius + 1.0;

    const int min_x = std::max(0, static_cast<int>(std::floor(std::min(x0, x1) - radius_plus_1)));
    const int max_x = std::min(width_ - 1, static_cast<int>(std::ceil(std::max(x0, x1) + radius_plus_1)));
    const int min_y = std::max(0, static_cast<int>(std::floor(std::min(y0, y1) - radius_plus_1)));
    const int max_y = std::min(height_ - 1, static_cast<int>(std::ceil(std::max(y0, y1) + radius_plus_1)));

    if (min_x > max_x || min_y > max_y) return;

    const double vx = x1 - x0;
    const double vy = y1 - y0;
    const double len_sq = vx * vx + vy * vy;
    const double base_alpha = color.a;

    for (int y = min_y; y <= max_y; ++y) {
        const double py = y + 0.5;
        const double w_y = py - y0;

        for (int x = min_x; x <= max_x; ++x) {
            const double px = x + 0.5;
            const double w_x = px - x0;

            double dist_sq = 0.0;
            if (len_sq < 1e-6) {
                dist_sq = w_x * w_x + w_y * w_y;
            } else {
                double t = (w_x * vx + w_y * vy) / len_sq;
                t = std::clamp(t, 0.0, 1.0);
                const double qx = x0 + t * vx;
                const double qy = y0 + t * vy;
                const double dx = px - qx;
                const double dy = py - qy;
                dist_sq = dx * dx + dy * dy;
            }

            const double dist = std::sqrt(dist_sq);
            if (dist < radius_plus_1) {
                const double coverage = std::clamp(0.5 + radius - dist, 0.0, 1.0);
                if (coverage > 0.0) {
                    const uint8_t alpha = static_cast<uint8_t>(std::round(base_alpha * coverage));
                    if (alpha > 0) {
                        blend_pixel(x, y, color.with_alpha(alpha));
                    }
                }
            }
        }
    }
}

void FrameBuffer::draw_dashed_line_aa(
    double x0, double y0, double x1, double y1,
    Color color, double thickness,
    double dash_len, double gap_len
) noexcept {
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    const double total_len = std::sqrt(dx * dx + dy * dy);
    if (total_len < 1e-4) return;

    const double unit_x = dx / total_len;
    const double unit_y = dy / total_len;
    const double step = dash_len + gap_len;

    double current_dist = 0.0;
    while (current_dist < total_len) {
        const double seg_start = current_dist;
        const double seg_end = std::min(total_len, current_dist + dash_len);
        if (seg_end > seg_start) {
            draw_line_aa(
                x0 + unit_x * seg_start, y0 + unit_y * seg_start,
                x0 + unit_x * seg_end,   y0 + unit_y * seg_end,
                color, thickness
            );
        }
        current_dist += step;
    }
}

void FrameBuffer::draw_rect(int x, int y, int w, int h, Color color) noexcept {
    if (w <= 0 || h <= 0) return;
    for (int i = x; i < x + w; ++i) {
        set_pixel(i, y, color);
        set_pixel(i, y + h - 1, color);
    }
    for (int j = y; j < y + h; ++j) {
        set_pixel(x, j, color);
        set_pixel(x + w - 1, j, color);
    }
}

void FrameBuffer::fill_rect(int x, int y, int w, int h, Color color) noexcept {
    const int x_start = std::max(0, x);
    const int x_end = std::min(width_, x + w);
    const int y_start = std::max(0, y);
    const int y_end = std::min(height_, y + h);

    if (x_start >= x_end || y_start >= y_end) return;

    const uint32_t val = color.to_bgra32();
    for (int j = y_start; j < y_end; ++j) {
        const size_t row = static_cast<size_t>(j) * width_;
        for (int i = x_start; i < x_end; ++i) {
            pixels_[row + i] = val;
        }
    }
}

void FrameBuffer::draw_circle(int cx, int cy, int radius, Color color) noexcept {
    if (radius <= 0) {
        set_pixel(cx, cy, color);
        return;
    }
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    auto plot8 = [&](int px, int py) {
        set_pixel(cx + px, cy + py, color);
        set_pixel(cx - px, cy + py, color);
        set_pixel(cx + px, cy - py, color);
        set_pixel(cx - px, cy - py, color);
        set_pixel(cx + py, cy + px, color);
        set_pixel(cx - py, cy + px, color);
        set_pixel(cx + py, cy - px, color);
        set_pixel(cx - py, cy - px, color);
    };

    while (y >= x) {
        plot8(x, y);
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void FrameBuffer::fill_circle(int cx, int cy, int radius, Color color) noexcept {
    if (radius <= 0) {
        set_pixel(cx, cy, color);
        return;
    }
    const int r2 = radius * radius;
    const int y_min = std::max(0, cy - radius);
    const int y_max = std::min(height_ - 1, cy + radius);

    for (int y = y_min; y <= y_max; ++y) {
        const int dy = y - cy;
        const int dx = static_cast<int>(std::sqrt(r2 - dy * dy));
        const int x_min = std::max(0, cx - dx);
        const int x_max = std::min(width_ - 1, cx + dx);
        for (int x = x_min; x <= x_max; ++x) {
            set_pixel(x, y, color);
        }
    }
}

void FrameBuffer::fill_circle_aa(double cx, double cy, double radius, Color color) noexcept {
    if (radius <= 0.0 || color.a == 0) return;

    const int min_x = std::max(0, static_cast<int>(std::floor(cx - radius - 1.0)));
    const int max_x = std::min(width_ - 1, static_cast<int>(std::ceil(cx + radius + 1.0)));
    const int min_y = std::max(0, static_cast<int>(std::floor(cy - radius - 1.0)));
    const int max_y = std::min(height_ - 1, static_cast<int>(std::ceil(cy + radius + 1.0)));

    const double base_alpha = color.a;

    for (int y = min_y; y <= max_y; ++y) {
        const double py = y + 0.5;
        const double dy = py - cy;
        for (int x = min_x; x <= max_x; ++x) {
            const double px = x + 0.5;
            const double dx = px - cx;
            const double dist = std::sqrt(dx * dx + dy * dy);
            if (dist < radius + 1.0) {
                const double coverage = std::clamp(0.5 + radius - dist, 0.0, 1.0);
                if (coverage > 0.0) {
                    const uint8_t alpha = static_cast<uint8_t>(std::round(base_alpha * coverage));
                    if (alpha > 0) {
                        blend_pixel(x, y, color.with_alpha(alpha));
                    }
                }
            }
        }
    }
}

void FrameBuffer::draw_circle_aa(double cx, double cy, double radius, Color color, double thickness) noexcept {
    if (radius <= 0.0 || thickness <= 0.0 || color.a == 0) return;

    const double half_thick = thickness * 0.5;
    const double outer_r = radius + half_thick + 1.0;

    const int min_x = std::max(0, static_cast<int>(std::floor(cx - outer_r)));
    const int max_x = std::min(width_ - 1, static_cast<int>(std::ceil(cx + outer_r)));
    const int min_y = std::max(0, static_cast<int>(std::floor(cy - outer_r)));
    const int max_y = std::min(height_ - 1, static_cast<int>(std::ceil(cy + outer_r)));

    const double base_alpha = color.a;

    for (int y = min_y; y <= max_y; ++y) {
        const double py = y + 0.5;
        const double dy = py - cy;
        for (int x = min_x; x <= max_x; ++x) {
            const double px = x + 0.5;
            const double dx = px - cx;
            const double dist = std::sqrt(dx * dx + dy * dy);
            const double delta = std::abs(dist - radius);
            if (delta < half_thick + 1.0) {
                const double coverage = std::clamp(0.5 + half_thick - delta, 0.0, 1.0);
                if (coverage > 0.0) {
                    const uint8_t alpha = static_cast<uint8_t>(std::round(base_alpha * coverage));
                    if (alpha > 0) {
                        blend_pixel(x, y, color.with_alpha(alpha));
                    }
                }
            }
        }
    }
}

void FrameBuffer::fill_rounded_rect(int x, int y, int w, int h, int radius, Color color) noexcept {
    if (w <= 0 || h <= 0) return;
    radius = std::clamp(radius, 0, std::min(w, h) / 2);
    if (radius <= 0) {
        fill_rect(x, y, w, h, color);
        return;
    }

    // Center body
    fill_rect(x + radius, y, w - 2 * radius, h, color);
    // Left & right strips
    fill_rect(x, y + radius, radius, h - 2 * radius, color);
    fill_rect(x + w - radius, y + radius, radius, h - 2 * radius, color);

    // 4 anti-aliased corners
    fill_circle_aa(x + radius,          y + radius,          radius, color);
    fill_circle_aa(x + w - radius - 1,  y + radius,          radius, color);
    fill_circle_aa(x + radius,          y + h - radius - 1,  radius, color);
    fill_circle_aa(x + w - radius - 1,  y + h - radius - 1,  radius, color);
}

void FrameBuffer::draw_rounded_rect(int x, int y, int w, int h, int radius, Color color, int thickness) noexcept {
    if (w <= 0 || h <= 0) return;
    radius = std::clamp(radius, 0, std::min(w, h) / 2);
    if (radius <= 0) {
        draw_rect(x, y, w, h, color);
        return;
    }

    const double t = static_cast<double>(thickness);
    // 4 straight edges
    draw_line_aa(x + radius, y, x + w - radius, y, color, t);
    draw_line_aa(x + radius, y + h - 1, x + w - radius, y + h - 1, color, t);
    draw_line_aa(x, y + radius, x, y + h - radius, color, t);
    draw_line_aa(x + w - 1, y + radius, x + w - 1, y + h - radius, color, t);
}

void FrameBuffer::draw_text(int x, int y, std::string_view text, Color color, int scale) noexcept {
    scale = std::max(1, scale);
    int cur_x = x;
    int cur_y = y;

    for (char c : text) {
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            cur_x = x;
            cur_y += (7 + 3) * scale;
            continue;
        }
        if (c < 32 || c > 126) {
            c = ' ';
        }
        const auto& glyph = kFont5x7[c - 32];
        for (int col = 0; col < 5; ++col) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; ++row) {
                if (bits & (1 << row)) {
                    if (scale == 1) {
                        set_pixel(cur_x + col, cur_y + row, color);
                    } else {
                        fill_rect(cur_x + col * scale, cur_y + row * scale, scale, scale, color);
                    }
                }
            }
        }
        cur_x += (5 + 1) * scale;
    }
}

void FrameBuffer::fill_triangle(double x0, double y0, double x1, double y1, double x2, double y2, Color color) noexcept {
    if (color.a == 0) return;

    int min_x = std::max(0, static_cast<int>(std::floor(std::min({x0, x1, x2}))));
    int max_x = std::min(width_ - 1, static_cast<int>(std::ceil(std::max({x0, x1, x2}))));
    int min_y = std::max(0, static_cast<int>(std::floor(std::min({y0, y1, y2}))));
    int max_y = std::min(height_ - 1, static_cast<int>(std::ceil(std::max({y0, y1, y2}))));
    if (min_x > max_x || min_y > max_y) return;

    auto orient = [](double ax, double ay, double bx, double by, double cx, double cy) -> double {
        return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    };

    double area = orient(x0, y0, x1, y1, x2, y2);
    if (std::abs(area) < 1e-6) return;

    if (area < 0.0) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }

    const double dx01 = x1 - x0, dy01 = y1 - y0;
    const double dx12 = x2 - x1, dy12 = y2 - y1;
    const double dx20 = x0 - x2, dy20 = y0 - y2;

    for (int y = min_y; y <= max_y; ++y) {
        const double py = y + 0.5;
        for (int x = min_x; x <= max_x; ++x) {
            const double px = x + 0.5;
            double w0 = dx12 * (py - y1) - dy12 * (px - x1);
            double w1 = dx20 * (py - y2) - dy20 * (px - x2);
            double w2 = dx01 * (py - y0) - dy01 * (px - x0);
            if (w0 >= 0.0 && w1 >= 0.0 && w2 >= 0.0) {
                if (color.a == 255) {
                    set_pixel(x, y, color);
                } else {
                    blend_pixel(x, y, color);
                }
            }
        }
    }
}

void FrameBuffer::copy_to_rgba(std::vector<uint8_t>& out_rgba) const {
    out_rgba.resize(pixel_count() * 4);
    for (size_t i = 0; i < pixel_count(); ++i) {
        const uint32_t bgra = pixels_[i];
        out_rgba[i * 4 + 0] = static_cast<uint8_t>((bgra >> 16) & 0xFF); // R
        out_rgba[i * 4 + 1] = static_cast<uint8_t>((bgra >> 8) & 0xFF);  // G
        out_rgba[i * 4 + 2] = static_cast<uint8_t>(bgra & 0xFF);         // B
        out_rgba[i * 4 + 3] = static_cast<uint8_t>((bgra >> 24) & 0xFF); // A
    }
}

} // namespace formulaic
