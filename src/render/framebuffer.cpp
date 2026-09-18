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

// Xiaolin Wu's Anti-Aliased Line Drawing
void FrameBuffer::draw_line_aa(double x0, double y0, double x1, double y1, Color color) noexcept {
    const bool steep = std::abs(y1 - y0) > std::abs(x0 - x1);
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    const double dx = x1 - x0;
    const double dy = y1 - y0;
    const double gradient = (dx == 0.0) ? 1.0 : (dy / dx);

    // Handle first endpoint
    double xend = std::round(x0);
    double yend = y0 + gradient * (xend - x0);
    double xgap = 1.0 - (x0 + 0.5 - std::floor(x0 + 0.5));
    const int xpxl1 = static_cast<int>(xend);
    const int ypxl1 = static_cast<int>(std::floor(yend));

    if (steep) {
        blend_pixel(ypxl1,     xpxl1, color.with_alpha(static_cast<uint8_t>(color.a * (1.0 - (yend - std::floor(yend))) * xgap)));
        blend_pixel(ypxl1 + 1, xpxl1, color.with_alpha(static_cast<uint8_t>(color.a * (yend - std::floor(yend)) * xgap)));
    } else {
        blend_pixel(xpxl1, ypxl1,     color.with_alpha(static_cast<uint8_t>(color.a * (1.0 - (yend - std::floor(yend))) * xgap)));
        blend_pixel(xpxl1, ypxl1 + 1, color.with_alpha(static_cast<uint8_t>(color.a * (yend - std::floor(yend)) * xgap)));
    }
    double intery = yend + gradient;

    // Handle second endpoint
    xend = std::round(x1);
    yend = y1 + gradient * (xend - x1);
    xgap = x1 + 0.5 - std::floor(x1 + 0.5);
    const int xpxl2 = static_cast<int>(xend);
    const int ypxl2 = static_cast<int>(std::floor(yend));

    if (steep) {
        blend_pixel(ypxl2,     xpxl2, color.with_alpha(static_cast<uint8_t>(color.a * (1.0 - (yend - std::floor(yend))) * xgap)));
        blend_pixel(ypxl2 + 1, xpxl2, color.with_alpha(static_cast<uint8_t>(color.a * (yend - std::floor(yend)) * xgap)));
    } else {
        blend_pixel(xpxl2, ypxl2,     color.with_alpha(static_cast<uint8_t>(color.a * (1.0 - (yend - std::floor(yend))) * xgap)));
        blend_pixel(xpxl2, ypxl2 + 1, color.with_alpha(static_cast<uint8_t>(color.a * (yend - std::floor(yend)) * xgap)));
    }

    // Main loop
    if (steep) {
        for (int x = xpxl1 + 1; x < xpxl2; ++x) {
            const int y = static_cast<int>(std::floor(intery));
            const double f = intery - y;
            blend_pixel(y,     x, color.with_alpha(static_cast<uint8_t>(color.a * (1.0 - f))));
            blend_pixel(y + 1, x, color.with_alpha(static_cast<uint8_t>(color.a * f)));
            intery += gradient;
        }
    } else {
        for (int x = xpxl1 + 1; x < xpxl2; ++x) {
            const int y = static_cast<int>(std::floor(intery));
            const double f = intery - y;
            blend_pixel(x, y,     color.with_alpha(static_cast<uint8_t>(color.a * (1.0 - f))));
            blend_pixel(x, y + 1, color.with_alpha(static_cast<uint8_t>(color.a * f)));
            intery += gradient;
        }
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

void FrameBuffer::draw_text(int x, int y, std::string_view text, Color color, int scale) noexcept {
    scale = std::max(1, scale);
    int cur_x = x;

    for (char c : text) {
        if (c < 32 || c > 126) {
            c = '?';
        }
        const auto& glyph = kFont5x7[c - 32];
        for (int col = 0; col < 5; ++col) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; ++row) {
                if (bits & (1 << row)) {
                    if (scale == 1) {
                        set_pixel(cur_x + col, y + row, color);
                    } else {
                        fill_rect(cur_x + col * scale, y + row * scale, scale, scale, color);
                    }
                }
            }
        }
        cur_x += (5 + 1) * scale;
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
