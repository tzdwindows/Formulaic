#include <Formulaic/render/raster_engine.hpp>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace formulaic {

namespace {

// Calculate "nice" round step for grid divisions
double calculate_nice_step(double range, int max_ticks) {
    if (range <= 0.0 || max_ticks <= 0) return 1.0;
    const double raw_step = range / max_ticks;
    const double magnitude = std::pow(10.0, std::floor(std::log10(raw_step)));
    const double residual = raw_step / magnitude;

    double nice_factor = 1.0;
    if (residual > 5.0) nice_factor = 10.0;
    else if (residual > 2.0) nice_factor = 5.0;
    else if (residual > 1.0) nice_factor = 2.0;

    return nice_factor * magnitude;
}

std::string format_coord_number(double val) {
    if (std::abs(val) < 1e-9) return "0";
    std::ostringstream ss;
    ss << std::setprecision(4) << val;
    return ss.str();
}

} // anonymous namespace

void RasterEngine::render_grid(
    FrameBuffer& fb,
    const Viewport& vp,
    const GridStyle& style,
    const PipelineHooks* hooks
) const {
    (void)hooks;
    const Rect2D& b = vp.bounds();
    const double x_step = calculate_nice_step(b.width(), style.major_divisions);
    const double y_step = calculate_nice_step(b.height(), style.major_divisions);

    if (style.show_grid) {
        // Minor grid lines
        const double x_minor_step = x_step / style.minor_divisions;
        const double y_minor_step = y_step / style.minor_divisions;

        const double x_minor_start = std::floor(b.x_min / x_minor_step) * x_minor_step;
        for (double x = x_minor_start; x <= b.x_max; x += x_minor_step) {
            Point2I pt_top = vp.world_to_screen({x, b.y_max});
            Point2I pt_bot = vp.world_to_screen({x, b.y_min});
            fb.draw_line(pt_top.x, 0, pt_bot.x, fb.height() - 1, style.minor_grid_color);
        }

        const double y_minor_start = std::floor(b.y_min / y_minor_step) * y_minor_step;
        for (double y = y_minor_start; y <= b.y_max; y += y_minor_step) {
            Point2I pt_left = vp.world_to_screen({b.x_min, y});
            Point2I pt_right = vp.world_to_screen({b.x_max, y});
            fb.draw_line(0, pt_left.y, fb.width() - 1, pt_right.y, style.minor_grid_color);
        }

        // Major grid lines
        const double x_major_start = std::floor(b.x_min / x_step) * x_step;
        for (double x = x_major_start; x <= b.x_max; x += x_step) {
            Point2I pt_top = vp.world_to_screen({x, b.y_max});
            Point2I pt_bot = vp.world_to_screen({x, b.y_min});
            fb.draw_line(pt_top.x, 0, pt_bot.x, fb.height() - 1, style.major_grid_color);
        }

        const double y_major_start = std::floor(b.y_min / y_step) * y_step;
        for (double y = y_major_start; y <= b.y_max; y += y_step) {
            Point2I pt_left = vp.world_to_screen({b.x_min, y});
            Point2I pt_right = vp.world_to_screen({b.x_max, y});
            fb.draw_line(0, pt_left.y, fb.width() - 1, pt_right.y, style.major_grid_color);
        }
    }

    if (style.show_axes) {
        Point2I origin_pt = vp.world_to_screen({0.0, 0.0});

        // X Axis (Horizontal)
        if (origin_pt.y >= 0 && origin_pt.y < fb.height()) {
            fb.draw_line(0, origin_pt.y, fb.width() - 1, origin_pt.y, style.axis_color, 2);
        }
        // Y Axis (Vertical)
        if (origin_pt.x >= 0 && origin_pt.x < fb.width()) {
            fb.draw_line(origin_pt.x, 0, origin_pt.x, fb.height() - 1, style.axis_color, 2);
        }
    }

    if (style.show_labels) {
        Point2I origin_pt = vp.world_to_screen({0.0, 0.0});
        const int label_y = std::clamp(origin_pt.y + 4, 10, fb.height() - 20);
        const int label_x = std::clamp(origin_pt.x + 4, 10, fb.width() - 60);

        // X-axis tick labels
        const double x_start = std::floor(b.x_min / x_step) * x_step;
        for (double x = x_start; x <= b.x_max; x += x_step) {
            if (std::abs(x) < 1e-9) continue;
            Point2I pt = vp.world_to_screen({x, 0.0});
            std::string text = format_coord_number(x);
            fb.draw_text(pt.x - static_cast<int>(text.size() * 3), label_y, text, style.text_color);
        }

        // Y-axis tick labels
        const double y_start = std::floor(b.y_min / y_step) * y_step;
        for (double y = y_start; y <= b.y_max; y += y_step) {
            if (std::abs(y) < 1e-9) continue;
            Point2I pt = vp.world_to_screen({0.0, y});
            std::string text = format_coord_number(y);
            fb.draw_text(label_x, pt.y - 4, text, style.text_color);
        }

        // Origin "0" label
        if (origin_pt.x >= 0 && origin_pt.x < fb.width() && origin_pt.y >= 0 && origin_pt.y < fb.height()) {
            fb.draw_text(origin_pt.x + 4, origin_pt.y + 4, "0", style.text_color);
        }
    }
}

void RasterEngine::plot_explicit(
    FrameBuffer& fb,
    const Viewport& vp,
    const Expression& expr,
    Color color,
    int line_thickness,
    double time_t,
    const PipelineHooks* hooks
) const {
    const int w = fb.width();
    const int h = fb.height();
    if (w <= 1 || h <= 1 || !expr.is_valid()) return;

    // Subpixel sampling: 2 samples per screen pixel column
    const int num_samples = w * 2;
    const double dx_screen = static_cast<double>(w - 1) / (num_samples - 1);

    bool prev_valid = false;
    Point2D prev_pt_screen{0.0, 0.0};
    double prev_y_world = 0.0;

    for (int i = 0; i < num_samples; ++i) {
        const double sx = i * dx_screen;
        Point2D world_pt = vp.screen_to_world(sx, 0.0);
        double y_val = expr.eval(world_pt.x, 0.0, time_t);

        if (std::isnan(y_val) || std::isinf(y_val)) {
            prev_valid = false;
            continue;
        }

        Point2D curve_world{world_pt.x, y_val};
        if (hooks && hooks->has_coord_transform_hook()) {
            curve_world = hooks->transform_coordinate(curve_world, vp);
        }

        Point2D curr_pt_screen = vp.world_to_screen_f(curve_world);

        // Detect vertical asymptote singularity (e.g. tan(x), 1/x)
        if (prev_valid) {
            const double dy_screen = std::abs(curr_pt_screen.y - prev_pt_screen.y);
            const bool sign_flip_large = (prev_y_world * y_val < 0.0) && (std::abs(y_val - prev_y_world) > vp.bounds().height() * 0.5);
            if (dy_screen > h * 0.8 || sign_flip_large) {
                // Skip line connecting asymptote jump
                prev_valid = false;
                continue;
            }

            Color draw_color = color;
            if (hooks && hooks->has_pixel_shader_hook()) {
                draw_color = hooks->shade_pixel(world_pt.x, y_val, y_val, color, time_t);
            }

            if (line_thickness <= 1) {
                fb.draw_line_aa(prev_pt_screen.x, prev_pt_screen.y, curr_pt_screen.x, curr_pt_screen.y, draw_color);
            } else {
                fb.draw_line(
                    static_cast<int>(std::round(prev_pt_screen.x)),
                    static_cast<int>(std::round(prev_pt_screen.y)),
                    static_cast<int>(std::round(curr_pt_screen.x)),
                    static_cast<int>(std::round(curr_pt_screen.y)),
                    draw_color,
                    line_thickness
                );
            }
        }

        prev_pt_screen = curr_pt_screen;
        prev_y_world = y_val;
        prev_valid = true;
    }
}

void RasterEngine::plot_parametric(
    FrameBuffer& fb,
    const Viewport& vp,
    const Expression& expr_x,
    const Expression& expr_y,
    double param_start,
    double param_end,
    int sample_count,
    Color color,
    int line_thickness,
    double time_t,
    const PipelineHooks* hooks
) const {
    if (sample_count < 2 || !expr_x.is_valid() || !expr_y.is_valid()) return;

    const double dt = (param_end - param_start) / (sample_count - 1);
    bool prev_valid = false;
    Point2D prev_pt_screen{0.0, 0.0};

    for (int i = 0; i < sample_count; ++i) {
        const double t = param_start + i * dt;
        double wx = expr_x.eval(t, 0.0, time_t);
        double wy = expr_y.eval(t, 0.0, time_t);

        if (std::isnan(wx) || std::isnan(wy) || std::isinf(wx) || std::isinf(wy)) {
            prev_valid = false;
            continue;
        }

        Point2D world_pt{wx, wy};
        if (hooks && hooks->has_coord_transform_hook()) {
            world_pt = hooks->transform_coordinate(world_pt, vp);
        }

        Point2D curr_pt_screen = vp.world_to_screen_f(world_pt);

        if (prev_valid) {
            Color draw_color = color;
            if (hooks && hooks->has_pixel_shader_hook()) {
                draw_color = hooks->shade_pixel(wx, wy, t, color, time_t);
            }

            if (line_thickness <= 1) {
                fb.draw_line_aa(prev_pt_screen.x, prev_pt_screen.y, curr_pt_screen.x, curr_pt_screen.y, draw_color);
            } else {
                fb.draw_line(
                    static_cast<int>(std::round(prev_pt_screen.x)),
                    static_cast<int>(std::round(prev_pt_screen.y)),
                    static_cast<int>(std::round(curr_pt_screen.x)),
                    static_cast<int>(std::round(curr_pt_screen.y)),
                    draw_color,
                    line_thickness
                );
            }
        }

        prev_pt_screen = curr_pt_screen;
        prev_valid = true;
    }
}

// Marching Squares Algorithm for Implicit Function Contours f(x, y, t) = 0
void RasterEngine::plot_implicit(
    FrameBuffer& fb,
    const Viewport& vp,
    const Expression& expr,
    Color color,
    int line_thickness,
    double time_t,
    const PipelineHooks* hooks
) const {
    if (!expr.is_valid()) return;

    // Grid step in screen pixels (2px grid for high quality subpixel contours)
    constexpr int kStep = 2;
    const int cols = (fb.width() + kStep - 1) / kStep + 1;
    const int rows = (fb.height() + kStep - 1) / kStep + 1;

    std::vector<double> grid(static_cast<size_t>(cols) * rows);

    // 1. Evaluate grid vertices
    for (int r = 0; r < rows; ++r) {
        const double sy = r * kStep;
        for (int c = 0; c < cols; ++c) {
            const double sx = c * kStep;
            Point2D wpt = vp.screen_to_world(sx, sy);
            if (hooks && hooks->has_coord_transform_hook()) {
                wpt = hooks->transform_coordinate(wpt, vp);
            }
            grid[static_cast<size_t>(r) * cols + c] = expr.eval(wpt.x, wpt.y, time_t);
        }
    }

    auto lerp_edge = [](double v1, double v2) -> double {
        const double denom = v2 - v1;
        if (std::abs(denom) < 1e-9) return 0.5;
        return std::clamp(-v1 / denom, 0.0, 1.0);
    };

    auto draw_seg = [&](Point2D p1, Point2D p2) {
        if (hooks && hooks->has_pixel_shader_hook()) {
            Point2D mid_world = vp.screen_to_world((p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5);
            Color c = hooks->shade_pixel(mid_world.x, mid_world.y, 0.0, color, time_t);
            fb.draw_line(static_cast<int>(p1.x), static_cast<int>(p1.y),
                         static_cast<int>(p2.x), static_cast<int>(p2.y), c, line_thickness);
        } else {
            fb.draw_line(static_cast<int>(p1.x), static_cast<int>(p1.y),
                         static_cast<int>(p2.x), static_cast<int>(p2.y), color, line_thickness);
        }
    };

    // 2. Marching squares cell evaluation
    for (int r = 0; r < rows - 1; ++r) {
        const double sy0 = r * kStep;
        const double sy1 = (r + 1) * kStep;

        for (int c = 0; c < cols - 1; ++c) {
            const double sx0 = c * kStep;
            const double sx1 = (c + 1) * kStep;

            const double v_tl = grid[static_cast<size_t>(r) * cols + c];
            const double v_tr = grid[static_cast<size_t>(r) * cols + (c + 1)];
            const double v_br = grid[static_cast<size_t>(r + 1) * cols + (c + 1)];
            const double v_bl = grid[static_cast<size_t>(r + 1) * cols + c];

            if (std::isnan(v_tl) || std::isnan(v_tr) || std::isnan(v_br) || std::isnan(v_bl)) continue;

            const uint8_t mask = ((v_tl > 0) ? 8 : 0) |
                                 ((v_tr > 0) ? 4 : 0) |
                                 ((v_br > 0) ? 2 : 0) |
                                 ((v_bl > 0) ? 1 : 0);

            if (mask == 0 || mask == 15) continue;

            // Interpolated edge points: top, right, bottom, left
            const Point2D pt_top{sx0 + lerp_edge(v_tl, v_tr) * (sx1 - sx0), sy0};
            const Point2D pt_right{sx1, sy0 + lerp_edge(v_tr, v_br) * (sy1 - sy0)};
            const Point2D pt_bottom{sx0 + lerp_edge(v_bl, v_br) * (sx1 - sx0), sy1};
            const Point2D pt_left{sx0, sy0 + lerp_edge(v_tl, v_bl) * (sy1 - sy0)};

            switch (mask) {
                case 1:  draw_seg(pt_left, pt_bottom); break;
                case 2:  draw_seg(pt_bottom, pt_right); break;
                case 3:  draw_seg(pt_left, pt_right); break;
                case 4:  draw_seg(pt_top, pt_right); break;
                case 5:  draw_seg(pt_left, pt_top); draw_seg(pt_bottom, pt_right); break;
                case 6:  draw_seg(pt_top, pt_bottom); break;
                case 7:  draw_seg(pt_left, pt_top); break;
                case 8:  draw_seg(pt_left, pt_top); break;
                case 9:  draw_seg(pt_top, pt_bottom); break;
                case 10: draw_seg(pt_top, pt_right); draw_seg(pt_left, pt_bottom); break;
                case 11: draw_seg(pt_top, pt_right); break;
                case 12: draw_seg(pt_left, pt_right); break;
                case 13: draw_seg(pt_bottom, pt_right); break;
                case 14: draw_seg(pt_left, pt_bottom); break;
                default: break;
            }
        }
    }
}

void RasterEngine::plot_scalar_field(
    FrameBuffer& fb,
    const Viewport& vp,
    const Expression& expr,
    ColormapType colormap,
    double z_min,
    double z_max,
    double time_t,
    const PipelineHooks* hooks
) const {
    const int w = fb.width();
    const int h = fb.height();
    if (w <= 0 || h <= 0 || !expr.is_valid()) return;

    const double z_range = (z_max != z_min) ? (z_max - z_min) : 1.0;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            Point2D wpt = vp.screen_to_world(x, y);
            if (hooks && hooks->has_coord_transform_hook()) {
                wpt = hooks->transform_coordinate(wpt, vp);
            }

            double z_val = expr.eval(wpt.x, wpt.y, time_t);
            if (std::isnan(z_val)) continue;

            const double t = (z_val - z_min) / z_range;
            Color base_color = sample_colormap(colormap, t);

            if (hooks && hooks->has_pixel_shader_hook()) {
                base_color = hooks->shade_pixel(wpt.x, wpt.y, z_val, base_color, time_t);
            }

            fb.set_pixel(x, y, base_color);
        }
    }
}

} // namespace formulaic
