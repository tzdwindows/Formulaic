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
            fb.draw_line_aa(0.0, static_cast<double>(origin_pt.y), static_cast<double>(fb.width() - 1), static_cast<double>(origin_pt.y), style.axis_color, 1.5);
        }
        // Y Axis (Vertical)
        if (origin_pt.x >= 0 && origin_pt.x < fb.width()) {
            fb.draw_line_aa(static_cast<double>(origin_pt.x), 0.0, static_cast<double>(origin_pt.x), static_cast<double>(fb.height() - 1), style.axis_color, 1.5);
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

// Marching Squares / Triangles Algorithm for Implicit Function Contours f(x, y, t) = 0
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

    // Grid step in screen pixels (2px step for high FPS, ultra-crisp contours)
    constexpr int kStep = 2;
    const int cols = (fb.width() + kStep - 1) / kStep + 1;
    const int rows = (fb.height() + kStep - 1) / kStep + 1;

    std::vector<double> grid(static_cast<size_t>(cols) * rows);
    std::vector<Point2D> grid_world(static_cast<size_t>(cols) * rows);

    auto eval_safe = [&](Point2D wpt) -> double {
        if (hooks && hooks->has_coord_transform_hook()) {
            wpt = hooks->transform_coordinate(wpt, vp);
        }
        double v = expr.eval(wpt.x, wpt.y, time_t);
        if (std::isnan(v) || std::isinf(v)) {
            // Nudge slightly in world coords if hitting exact pole (e.g. x=0 or y=0)
            const double eps_x = (wpt.x >= 0.0) ? 1e-7 : -1e-7;
            const double eps_y = (wpt.y >= 0.0) ? 1e-7 : -1e-7;
            Point2D nudged{wpt.x + eps_x, wpt.y + eps_y};
            if (hooks && hooks->has_coord_transform_hook()) {
                nudged = hooks->transform_coordinate(nudged, vp);
            }
            v = expr.eval(nudged.x, nudged.y, time_t);
            if (std::isnan(v) || std::isinf(v)) {
                v = (v > 0.0 || (wpt.x >= 0.0 && wpt.y >= 0.0)) ? 1e7 : -1e7;
            }
        }
        if (v > 1e7) return 1e7;
        if (v < -1e7) return -1e7;
        return v;
    };

    // 1. Evaluate grid corners
    for (int r = 0; r < rows; ++r) {
        const double sy = r * kStep;
        for (int c = 0; c < cols; ++c) {
            const double sx = c * kStep;
            Point2D wpt = vp.screen_to_world(sx, sy);
            const size_t idx = static_cast<size_t>(r) * cols + c;
            grid_world[idx] = wpt;
            grid[idx] = eval_safe(wpt);
        }
    }

    struct VertexInfo {
        Point2D screen;
        Point2D world;
        double val;
    };

    auto check_edge = [&](const VertexInfo& A, const VertexInfo& B) -> std::pair<bool, Point2D> {
        if ((A.val > 0.0) == (B.val > 0.0)) return {false, {0.0, 0.0}};

        double t_low = 0.0;
        double t_high = 1.0;
        double v_low = A.val;
        double v_high = B.val;
        bool hit_singularity = false;

        for (int iter = 0; iter < 12; ++iter) {
            double t_mid = 0.5 * (t_low + t_high);
            double px = A.world.x + t_mid * (B.world.x - A.world.x);
            double py = A.world.y + t_mid * (B.world.y - A.world.y);
            Point2D mid_wpt{px, py};
            if (hooks && hooks->has_coord_transform_hook()) {
                mid_wpt = hooks->transform_coordinate(mid_wpt, vp);
            }
            double vm = expr.eval(mid_wpt.x, mid_wpt.y, time_t);
            if (std::isnan(vm) || std::isinf(vm)) {
                hit_singularity = true;
                break;
            }
            if ((vm > 0.0) == (v_low > 0.0)) {
                t_low = t_mid;
                v_low = vm;
            } else {
                t_high = t_mid;
                v_high = vm;
            }
        }
        if (hit_singularity) return {false, {0.0, 0.0}};

        double t = 0.5 * (t_low + t_high);
        Point2D root_world{
            A.world.x + t * (B.world.x - A.world.x),
            A.world.y + t * (B.world.y - A.world.y)
        };
        Point2D eval_root = root_world;
        if (hooks && hooks->has_coord_transform_hook()) {
            eval_root = hooks->transform_coordinate(eval_root, vp);
        }
        double vp_val = expr.eval(eval_root.x, eval_root.y, time_t);
        if (std::isnan(vp_val) || std::isinf(vp_val)) return {false, {0.0, 0.0}};

        // Pole check 1: Residual at bisection root
        const double min_corner = std::min(std::abs(A.val), std::abs(B.val));
        if (std::abs(vp_val) > 2.0 && std::abs(vp_val) >= 0.5 * min_corner) {
            return {false, {0.0, 0.0}};
        }

        // Pole check 2: Directional derivative vs secant
        const double dx = B.world.x - A.world.x;
        const double dy = B.world.y - A.world.y;
        const double dist = std::hypot(dx, dy);
        if (dist > 1e-12) {
            const double ux = dx / dist;
            const double uy = dy / dist;
            const double secant = (B.val - A.val) / dist;
            const double eps = 1e-5;

            const double pAx = A.world.x + 0.1 * dx;
            const double pAy = A.world.y + 0.1 * dy;
            Point2D ptA_plus{pAx + eps * ux, pAy + eps * uy};
            Point2D ptA_minus{pAx - eps * ux, pAy - eps * uy};
            if (hooks && hooks->has_coord_transform_hook()) {
                ptA_plus = hooks->transform_coordinate(ptA_plus, vp);
                ptA_minus = hooks->transform_coordinate(ptA_minus, vp);
            }
            const double dfA = (expr.eval(ptA_plus.x, ptA_plus.y, time_t) -
                                expr.eval(ptA_minus.x, ptA_minus.y, time_t)) / (2.0 * eps);

            const double pBx = B.world.x - 0.1 * dx;
            const double pBy = B.world.y - 0.1 * dy;
            Point2D ptB_plus{pBx + eps * ux, pBy + eps * uy};
            Point2D ptB_minus{pBx - eps * ux, pBy - eps * uy};
            if (hooks && hooks->has_coord_transform_hook()) {
                ptB_plus = hooks->transform_coordinate(ptB_plus, vp);
                ptB_minus = hooks->transform_coordinate(ptB_minus, vp);
            }
            const double dfB = (expr.eval(ptB_plus.x, ptB_plus.y, time_t) -
                                expr.eval(ptB_minus.x, ptB_minus.y, time_t)) / (2.0 * eps);

            if (!std::isnan(dfA) && !std::isnan(dfB) && !std::isinf(dfA) && !std::isinf(dfB)) {
                if ((secant * dfA < 0.0) && (secant * dfB < 0.0)) {
                    return {false, {0.0, 0.0}};
                }
            }
        }

        Point2D root_screen{
            A.screen.x + t * (B.screen.x - A.screen.x),
            A.screen.y + t * (B.screen.y - A.screen.y)
        };
        return {true, root_screen};
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
        fb.draw_line_aa(p1.x, p1.y, p2.x, p2.y, c, line_thickness);
    };

    Point2D origin_screen = vp.world_to_screen_f({0.0, 0.0});
    bool has_x_axis = (origin_screen.y >= 0.0 && origin_screen.y < fb.height());
    bool has_y_axis = (origin_screen.x >= 0.0 && origin_screen.x < fb.width());

    // 2. Iterate each cell and process 4 triangles with Center
    for (int r = 0; r < rows - 1; ++r) {
        const double sy0 = r * kStep;
        const double sy1 = (r + 1) * kStep;

        for (int c = 0; c < cols - 1; ++c) {
            const double sx0 = c * kStep;
            const double sx1 = (c + 1) * kStep;

            auto process_subcell = [&](double sub_sx0, double sub_sy0, double sub_sx1, double sub_sy1) {
                const double sub_sx_mid = (sub_sx0 + sub_sx1) * 0.5;
                const double sub_sy_mid = (sub_sy0 + sub_sy1) * 0.5;

                Point2D w_tl = vp.screen_to_world(sub_sx0, sub_sy0);
                Point2D w_tr = vp.screen_to_world(sub_sx1, sub_sy0);
                Point2D w_br = vp.screen_to_world(sub_sx1, sub_sy1);
                Point2D w_bl = vp.screen_to_world(sub_sx0, sub_sy1);
                Point2D w_c  = vp.screen_to_world(sub_sx_mid, sub_sy_mid);

                VertexInfo vTL{{sub_sx0, sub_sy0}, w_tl, eval_safe(w_tl)};
                VertexInfo vTR{{sub_sx1, sub_sy0}, w_tr, eval_safe(w_tr)};
                VertexInfo vBR{{sub_sx1, sub_sy1}, w_br, eval_safe(w_br)};
                VertexInfo vBL{{sub_sx0, sub_sy1}, w_bl, eval_safe(w_bl)};
                VertexInfo vC {{sub_sx_mid, sub_sy_mid}, w_c, eval_safe(w_c)};

                auto local_draw = [&](const VertexInfo& va, const VertexInfo& vb, const VertexInfo& vc) {
                    Point2D pts[3];
                    int count = 0;
                    auto [ok01, p01] = check_edge(va, vb);
                    if (ok01) pts[count++] = p01;
                    auto [ok12, p12] = check_edge(vb, vc);
                    if (ok12) pts[count++] = p12;
                    auto [ok20, p20] = check_edge(vc, va);
                    if (ok20) pts[count++] = p20;
                    if (count == 2) {
                        draw_seg(pts[0], pts[1]);
                    }
                };

                local_draw(vTL, vTR, vC);
                local_draw(vTR, vBR, vC);
                local_draw(vBR, vBL, vC);
                local_draw(vBL, vTL, vC);
            };

            // Check if this cell is intersected by the X-axis (y = 0) or Y-axis (x = 0)
            bool crosses_x_axis = (has_x_axis && origin_screen.y > sy0 + 0.05 && origin_screen.y < sy1 - 0.05);
            bool crosses_y_axis = (has_y_axis && origin_screen.x > sx0 + 0.05 && origin_screen.x < sx1 - 0.05);

            if (crosses_x_axis && !crosses_y_axis) {
                // Split vertically into top (y > 0) and bottom (y < 0) along the axis
                const double ay = origin_screen.y;
                const double d = 0.001; // tiny subpixel offset into each half-plane
                process_subcell(sx0, sy0, sx1, ay - d);
                process_subcell(sx0, ay + d, sx1, sy1);
            } else if (crosses_y_axis && !crosses_x_axis) {
                // Split horizontally into left (x < 0) and right (x > 0) along the axis
                const double ax = origin_screen.x;
                const double d = 0.001;
                process_subcell(sx0, sy0, ax - d, sy1);
                process_subcell(ax + d, sy0, sx1, sy1);
            } else if (crosses_x_axis && crosses_y_axis) {
                // Cell contains the origin (0, 0): split into 4 quadrants
                const double ax = origin_screen.x;
                const double ay = origin_screen.y;
                const double d = 0.001;
                process_subcell(sx0, sy0, ax - d, ay - d);
                process_subcell(ax + d, sy0, sx1, ay - d);
                process_subcell(sx0, ay + d, ax - d, sy1);
                process_subcell(ax + d, ay + d, sx1, sy1);
            } else {
                // Standard cell
                const size_t idx_tl = static_cast<size_t>(r) * cols + c;
                const size_t idx_tr = static_cast<size_t>(r) * cols + (c + 1);
                const size_t idx_br = static_cast<size_t>(r + 1) * cols + (c + 1);
                const size_t idx_bl = static_cast<size_t>(r + 1) * cols + c;

                VertexInfo vTL{{sx0, sy0}, grid_world[idx_tl], grid[idx_tl]};
                VertexInfo vTR{{sx1, sy0}, grid_world[idx_tr], grid[idx_tr]};
                VertexInfo vBR{{sx1, sy1}, grid_world[idx_br], grid[idx_br]};
                VertexInfo vBL{{sx0, sy1}, grid_world[idx_bl], grid[idx_bl]};

                const double sx_mid = (sx0 + sx1) * 0.5;
                const double sy_mid = (sy0 + sy1) * 0.5;
                Point2D w_mid = vp.screen_to_world(sx_mid, sy_mid);
                VertexInfo vC{{sx_mid, sy_mid}, w_mid, eval_safe(w_mid)};

                auto local_draw = [&](const VertexInfo& va, const VertexInfo& vb, const VertexInfo& vc) {
                    Point2D pts[3];
                    int count = 0;
                    auto [ok01, p01] = check_edge(va, vb);
                    if (ok01) pts[count++] = p01;
                    auto [ok12, p12] = check_edge(vb, vc);
                    if (ok12) pts[count++] = p12;
                    auto [ok20, p20] = check_edge(vc, va);
                    if (ok20) pts[count++] = p20;
                    if (count == 2) {
                        draw_seg(pts[0], pts[1]);
                    }
                };

                local_draw(vTL, vTR, vC);
                local_draw(vTR, vBR, vC);
                local_draw(vBR, vBL, vC);
                local_draw(vBL, vTL, vC);
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
