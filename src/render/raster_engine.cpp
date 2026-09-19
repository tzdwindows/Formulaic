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
    if (style.background_color.a > 0) {
        fb.clear(style.background_color);
    }

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
    double line_thickness,
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

            // Always use distance-field sub-pixel anti-aliasing with continuous thickness
            fb.draw_line_aa(prev_pt_screen.x, prev_pt_screen.y, curr_pt_screen.x, curr_pt_screen.y, draw_color, line_thickness);
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
    double line_thickness,
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

            fb.draw_line_aa(prev_pt_screen.x, prev_pt_screen.y, curr_pt_screen.x, curr_pt_screen.y, draw_color, line_thickness);
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
    double line_thickness,
    double time_t,
    const PipelineHooks* hooks
) const {
    if (!expr.is_valid()) return;

    // Grid step in screen pixels (3px grid with sub-pixel interpolation for high FPS smooth contours)
    constexpr int kStep = 3;
    const int cols = (fb.width() + kStep - 1) / kStep + 1;
    const int rows = (fb.height() + kStep - 1) / kStep + 1;

    std::vector<double> grid(static_cast<size_t>(cols) * rows);
    std::vector<Point2D> grid_world(static_cast<size_t>(cols) * rows);

    // 1. Evaluate grid vertices
    for (int r = 0; r < rows; ++r) {
        const double sy = r * kStep;
        for (int c = 0; c < cols; ++c) {
            const double sx = c * kStep;
            Point2D wpt = vp.screen_to_world(sx, sy);
            if (hooks && hooks->has_coord_transform_hook()) {
                wpt = hooks->transform_coordinate(wpt, vp);
            }
            const size_t idx = static_cast<size_t>(r) * cols + c;
            grid_world[idx] = wpt;
            grid[idx] = expr.eval(wpt.x, wpt.y, time_t);
        }
    }

    auto check_edge = [&](double vA, double vB, const Point2D& wA, const Point2D& wB) -> std::pair<bool, double> {
        if ((vA > 0) == (vB > 0)) return {false, 0.5};
        const double denom = vB - vA;
        if (std::abs(denom) < 1e-9) return {false, 0.5};
        const double t = std::clamp(-vA / denom, 0.0, 1.0);

        // Evaluate candidate root in world coordinates
        const double wpx = wA.x + t * (wB.x - wA.x);
        const double wpy = wA.y + t * (wB.y - wA.y);
        const double vp_val = expr.eval(wpx, wpy, time_t);

        if (std::isnan(vp_val) || std::isinf(vp_val)) {
            return {false, t};
        }

        // Singularity / Pole check 1:
        // On a continuous function crossing zero, the residual at the linear interpolation root
        // must drop significantly compared to the corner values. If |vp_val| >= min(|vA|, |vB|) and is non-trivial,
        // it signifies a jump discontinuity / pole rather than a true zero crossing.
        const double min_corner = std::min(std::abs(vA), std::abs(vB));
        if (std::abs(vp_val) >= min_corner && std::abs(vp_val) > 1.0) {
            return {false, t};
        }

        // Singularity / Pole check 2: Directional derivative vs secant slope
        // Across an asymptotic pole (e.g. 1/x jumping from -inf to +inf), the secant is positive,
        // but the actual derivative d/dx(1/x) = -1/x^2 is negative on both sides.
        const double dx = wB.x - wA.x;
        const double dy = wB.y - wA.y;
        const double dist = std::hypot(dx, dy);
        if (dist > 1e-12) {
            const double ux = dx / dist;
            const double uy = dy / dist;
            const double secant = (vB - vA) / dist;
            const double eps = 1e-5;

            const double pAx = wA.x + 0.1 * dx;
            const double pAy = wA.y + 0.1 * dy;
            const double dfA = (expr.eval(pAx + eps * ux, pAy + eps * uy, time_t) -
                                expr.eval(pAx - eps * ux, pAy - eps * uy, time_t)) / (2.0 * eps);

            const double pBx = wB.x - 0.1 * dx;
            const double pBy = wB.y - 0.1 * dy;
            const double dfB = (expr.eval(pBx + eps * ux, pBy + eps * uy, time_t) -
                                expr.eval(pBx - eps * ux, pBy - eps * uy, time_t)) / (2.0 * eps);

            if (!std::isnan(dfA) && !std::isnan(dfB) && !std::isinf(dfA) && !std::isinf(dfB)) {
                if ((secant * dfA < 0.0) && (secant * dfB < 0.0)) {
                    return {false, t};
                }
            }
        }

        return {true, t};
    };

    auto draw_seg = [&](Point2D p1, Point2D p2) {
        Point2D mid_world = vp.screen_to_world((p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5);
        if (hooks && hooks->has_coord_transform_hook()) {
            mid_world = hooks->transform_coordinate(mid_world, vp);
        }
        const double vm = expr.eval(mid_world.x, mid_world.y, time_t);
        if (std::isnan(vm) || std::isinf(vm)) return;

        Color c = color;
        if (hooks && hooks->has_pixel_shader_hook()) {
            c = hooks->shade_pixel(mid_world.x, mid_world.y, 0.0, color, time_t);
        }
        // Sub-pixel floating-point anti-aliased segment
        fb.draw_line_aa(p1.x, p1.y, p2.x, p2.y, c, line_thickness);
    };

    // 2. Marching squares cell evaluation
    for (int r = 0; r < rows - 1; ++r) {
        const double sy0 = r * kStep;
        const double sy1 = (r + 1) * kStep;

        for (int c = 0; c < cols - 1; ++c) {
            const double sx0 = c * kStep;
            const double sx1 = (c + 1) * kStep;

            const size_t idx_tl = static_cast<size_t>(r) * cols + c;
            const size_t idx_tr = static_cast<size_t>(r) * cols + (c + 1);
            const size_t idx_br = static_cast<size_t>(r + 1) * cols + (c + 1);
            const size_t idx_bl = static_cast<size_t>(r + 1) * cols + c;

            const double v_tl = grid[idx_tl];
            const double v_tr = grid[idx_tr];
            const double v_br = grid[idx_br];
            const double v_bl = grid[idx_bl];

            if (std::isnan(v_tl) || std::isnan(v_tr) || std::isnan(v_br) || std::isnan(v_bl)) continue;
            if (std::isinf(v_tl) || std::isinf(v_tr) || std::isinf(v_br) || std::isinf(v_bl)) continue;

            const uint8_t mask = ((v_tl > 0) ? 8 : 0) |
                                 ((v_tr > 0) ? 4 : 0) |
                                 ((v_br > 0) ? 2 : 0) |
                                 ((v_bl > 0) ? 1 : 0);

            if (mask == 0 || mask == 15) continue;

            const auto& w_tl = grid_world[idx_tl];
            const auto& w_tr = grid_world[idx_tr];
            const auto& w_br = grid_world[idx_br];
            const auto& w_bl = grid_world[idx_bl];

            const auto [valid_top, t_top] = check_edge(v_tl, v_tr, w_tl, w_tr);
            const auto [valid_right, t_right] = check_edge(v_tr, v_br, w_tr, w_br);
            const auto [valid_bottom, t_bottom] = check_edge(v_bl, v_br, w_bl, w_br);
            const auto [valid_left, t_left] = check_edge(v_tl, v_bl, w_tl, w_bl);

            const Point2D pt_top{sx0 + t_top * (sx1 - sx0), sy0};
            const Point2D pt_right{sx1, sy0 + t_right * (sy1 - sy0)};
            const Point2D pt_bottom{sx0 + t_bottom * (sx1 - sx0), sy1};
            const Point2D pt_left{sx0, sy0 + t_left * (sy1 - sy0)};

            switch (mask) {
                case 1:  if (valid_left && valid_bottom) draw_seg(pt_left, pt_bottom); break;
                case 2:  if (valid_bottom && valid_right) draw_seg(pt_bottom, pt_right); break;
                case 3:  if (valid_left && valid_right) draw_seg(pt_left, pt_right); break;
                case 4:  if (valid_top && valid_right) draw_seg(pt_top, pt_right); break;
                case 5:
                    if (valid_left && valid_top) draw_seg(pt_left, pt_top);
                    if (valid_bottom && valid_right) draw_seg(pt_bottom, pt_right);
                    break;
                case 6:  if (valid_top && valid_bottom) draw_seg(pt_top, pt_bottom); break;
                case 7:  if (valid_left && valid_top) draw_seg(pt_left, pt_top); break;
                case 8:  if (valid_left && valid_top) draw_seg(pt_left, pt_top); break;
                case 9:  if (valid_top && valid_bottom) draw_seg(pt_top, pt_bottom); break;
                case 10:
                    if (valid_top && valid_right) draw_seg(pt_top, pt_right);
                    if (valid_left && valid_bottom) draw_seg(pt_left, pt_bottom);
                    break;
                case 11: if (valid_top && valid_right) draw_seg(pt_top, pt_right); break;
                case 12: if (valid_left && valid_right) draw_seg(pt_left, pt_right); break;
                case 13: if (valid_bottom && valid_right) draw_seg(pt_bottom, pt_right); break;
                case 14: if (valid_left && valid_bottom) draw_seg(pt_left, pt_bottom); break;
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

HitTestResult RasterEngine::hit_test_explicit(
    const Viewport& vp,
    const Expression& expr,
    Point2I mouse_screen,
    double tolerance_screen_px,
    double time_t,
    std::string_view name
) const {
    HitTestResult res;
    if (!expr.is_valid() || mouse_screen.x < 0 || mouse_screen.x >= vp.width() ||
        mouse_screen.y < 0 || mouse_screen.y >= vp.height()) {
        return res;
    }

    Point2D mouse_world = vp.screen_to_world(mouse_screen.x, mouse_screen.y);
    double curve_y = expr.eval(mouse_world.x, 0.0, time_t);

    if (std::isnan(curve_y) || std::isinf(curve_y)) {
        return res;
    }

    Point2I curve_screen = vp.world_to_screen({mouse_world.x, curve_y});
    double min_dist_px = std::abs(curve_screen.y - mouse_screen.y);
    double best_wx = mouse_world.x;
    double best_wy = curve_y;
    Point2I best_screen = curve_screen;

    // Search small horizontal neighborhood (+/- 4 px) to find minimum distance on steep curves
    for (int offset_px = -4; offset_px <= 4; ++offset_px) {
        if (offset_px == 0) continue;
        int sx = mouse_screen.x + offset_px;
        if (sx < 0 || sx >= vp.width()) continue;
        Point2D w_pt = vp.screen_to_world(sx, mouse_screen.y);
        double wy = expr.eval(w_pt.x, 0.0, time_t);
        if (std::isnan(wy) || std::isinf(wy)) continue;
        Point2I s_pt = vp.world_to_screen({w_pt.x, wy});
        double d = std::sqrt(static_cast<double>((s_pt.x - mouse_screen.x) * (s_pt.x - mouse_screen.x) +
                                                (s_pt.y - mouse_screen.y) * (s_pt.y - mouse_screen.y)));
        if (d < min_dist_px) {
            min_dist_px = d;
            best_wx = w_pt.x;
            best_wy = wy;
            best_screen = s_pt;
        }
    }

    if (min_dist_px <= tolerance_screen_px) {
        res.hit = true;
        res.world_pos = {best_wx, best_wy};
        res.screen_pos = best_screen;
        res.value = best_wy;
        res.distance_screen_px = min_dist_px;
        res.expr_name = std::string(name);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3);
        oss << res.expr_name << ": x=" << best_wx << ", y=" << best_wy;
        res.formatted_info = oss.str();
    }

    return res;
}

void RasterEngine::render_hover_indicator(
    FrameBuffer& fb,
    const Viewport& vp,
    const HitTestResult& hit,
    Color highlight_color
) const {
    if (!hit.hit) return;

    // 1. Dashed projection lines from hovered curve point to axes
    Point2I x_axis_pt = vp.world_to_screen({hit.world_pos.x, 0.0});
    Point2I y_axis_pt = vp.world_to_screen({0.0, hit.world_pos.y});

    Color guide_color(160, 170, 200, 160);
    fb.draw_dashed_line_aa(hit.screen_pos.x, hit.screen_pos.y, hit.screen_pos.x, x_axis_pt.y, guide_color, 1.2, 4.0, 3.0);
    fb.draw_dashed_line_aa(hit.screen_pos.x, hit.screen_pos.y, y_axis_pt.x, hit.screen_pos.y, guide_color, 1.2, 4.0, 3.0);

    // 2. Snapped glowing marker dot
    fb.fill_circle_aa(hit.screen_pos.x, hit.screen_pos.y, 9.0, highlight_color.with_alpha(90)); // outer glow halo
    fb.fill_circle_aa(hit.screen_pos.x, hit.screen_pos.y, 4.5, Color::White);                   // center bright core
    fb.draw_circle_aa(hit.screen_pos.x, hit.screen_pos.y, 4.5, highlight_color, 1.5);           // crisp ring

    // 3. Floating rounded interactive callout badge
    constexpr int badge_w = 145;
    constexpr int badge_h = 56;

    int bx = hit.screen_pos.x + 14;
    int by = hit.screen_pos.y - badge_h - 10;

    if (bx + badge_w > fb.width() - 8) {
        bx = hit.screen_pos.x - badge_w - 14;
    }
    if (by < 8) {
        by = hit.screen_pos.y + 14;
    }

    // Frosted glass background
    fb.fill_rounded_rect(bx, by, badge_w, badge_h, 6, Color(20, 22, 30, 235));
    fb.draw_rounded_rect(bx, by, badge_w, badge_h, 6, highlight_color.with_alpha(200), 1);

    // Text info inside callout badge
    fb.draw_text(bx + 10, by + 8, hit.expr_name, highlight_color);

    std::ostringstream sx;
    sx << std::fixed << std::setprecision(3) << "X: " << hit.world_pos.x;
    fb.draw_text(bx + 10, by + 23, sx.str(), Color::LightGray);

    std::ostringstream sy;
    sy << std::fixed << std::setprecision(3) << "Y: " << hit.world_pos.y;
    fb.draw_text(bx + 10, by + 38, sy.str(), Color::LightGray);
}

} // namespace formulaic
