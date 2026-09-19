#pragma once

#include <Formulaic/core/export.hpp>
#include <Formulaic/hooks/pipeline_hooks.hpp>
#include <Formulaic/parser/expression.hpp>
#include <Formulaic/render/color.hpp>
#include <Formulaic/render/framebuffer.hpp>
#include <Formulaic/render/viewport.hpp>

namespace formulaic {

struct FORMULAIC_API GridStyle {
    Color background_color{Color::BackgroundDark};
    Color major_grid_color{Color::GridGray};
    Color minor_grid_color{Color(35, 35, 45, 255)};
    Color axis_color{Color::AxisGray};
    Color text_color{Color::LightGray};
    bool show_grid{true};
    bool show_axes{true};
    bool show_labels{true};
    int major_divisions{10};
    int minor_divisions{5};
};

struct FORMULAIC_API Surface3DStyle {
    double azimuth_deg{45.0};      // Yaw rotation angle around Z axis in degrees
    double elevation_deg{30.0};    // Pitch elevation angle above XY plane in degrees
    double zoom{1.0};              // Camera zoom multiplier
    int grid_resolution_x{55};     // Mesh sample density in X
    int grid_resolution_y{55};     // Mesh sample density in Y
    double x_min{-5.0};
    double x_max{5.0};
    double y_min{-5.0};
    double y_max{5.0};
    double z_min{-1.0};
    double z_max{1.0};
    bool auto_z_range{true};       // Dynamically adjust z_min and z_max from sampled values
    bool show_wireframe{true};     // Render polygon wireframe grid lines
    bool show_mesh_faces{true};    // Render filled shaded polygons
    bool show_box_axes{true};      // Render 3D coordinate bounding box & axis lines
    ColormapType colormap{ColormapType::Viridis};
    Color wireframe_color{Color(30, 35, 50, 160)};
    Color axis_color{Color(130, 140, 165, 220)};
    Color text_color{Color::LightGray};
};

struct HitTestResult {
    bool hit{false};
    Point2D world_pos{0.0, 0.0};
    Point2I screen_pos{0, 0};
    double value{0.0};
    double distance_screen_px{0.0};
    std::string expr_name;
    std::string formatted_info;

    [[nodiscard]] HoverInfo to_hover_info() const {
        HoverInfo info{};
        info.is_hovered = hit;
        info.target_name = expr_name;
        info.world_pos = world_pos;
        info.screen_pos = screen_pos;
        info.value = value;
        info.distance_px = distance_screen_px;
        info.detail_text = formatted_info;
        return info;
    }
};

class FORMULAIC_API RasterEngine {
public:
    RasterEngine() = default;

    // Renders the coordinate axes, major/minor grid lines, and numerical labels
    void render_grid(
        FrameBuffer& fb,
        const Viewport& vp,
        const GridStyle& style = GridStyle{},
        const PipelineHooks* hooks = nullptr
    ) const;

    // Plots an explicit 1D function y = f(x, t) with high quality anti-aliasing
    void plot_explicit(
        FrameBuffer& fb,
        const Viewport& vp,
        const Expression& expr,
        Color color = Color::NeonBlue,
        double line_thickness = 2.0,
        double time_t = 0.0,
        const PipelineHooks* hooks = nullptr
    ) const;

    // Plots a 2D parametric curve x = fx(t), y = fy(t) with anti-aliasing
    void plot_parametric(
        FrameBuffer& fb,
        const Viewport& vp,
        const Expression& expr_x,
        const Expression& expr_y,
        double param_start = 0.0,
        double param_end = 6.283185307179586,
        int sample_count = 1000,
        Color color = Color::NeonPink,
        double line_thickness = 2.0,
        double time_t = 0.0,
        const PipelineHooks* hooks = nullptr
    ) const;

    // Plots an implicit function f(x, y, t) = 0 using sub-pixel Marching Squares with anti-aliasing
    void plot_implicit(
        FrameBuffer& fb,
        const Viewport& vp,
        const Expression& expr,
        Color color = Color::NeonGreen,
        double line_thickness = 2.0,
        double time_t = 0.0,
        const PipelineHooks* hooks = nullptr
    ) const;

    // Plots a 2D scalar field z = f(x, y, t) as a shaded heatmap
    void plot_scalar_field(
        FrameBuffer& fb,
        const Viewport& vp,
        const Expression& expr,
        ColormapType colormap = ColormapType::Viridis,
        double z_min = -1.0,
        double z_max = 1.0,
        double time_t = 0.0,
        const PipelineHooks* hooks = nullptr
    ) const;

    // Plots a 3D surface z = f(x, y, t) with projection, depth sorting, shading, and bounding box
    void plot_surface_3d(
        FrameBuffer& fb,
        const Expression& expr,
        const Surface3DStyle& style = Surface3DStyle{},
        double time_t = 0.0,
        const PipelineHooks* hooks = nullptr
    ) const;

    // Hit tests an explicit function against mouse cursor in screen space
    [[nodiscard]] HitTestResult hit_test_explicit(
        const Viewport& vp,
        const Expression& expr,
        Point2I mouse_screen,
        double tolerance_screen_px = 14.0,
        double time_t = 0.0,
        std::string_view name = "f(x)"
    ) const;

    // Draws interactive hover highlight: snapped dot, axis guide lines, and floating info badge
    void render_hover_indicator(
        FrameBuffer& fb,
        const Viewport& vp,
        const HitTestResult& hit,
        Color highlight_color = Color::White
    ) const;

    // Plots a mathematical curve or field directly from a standard LaTeX formula
    void plot_latex(
        FrameBuffer& fb,
        const Viewport& vp,
        std::string_view latex_text,
        Color color = Color::NeonPink,
        double line_thickness = 2.0,
        double time_t = 0.0,
        const PipelineHooks* hooks = nullptr
    ) const;

    // Renders visual LaTeXLive mathematical formula banner card on framebuffer
    void plot_latex_card(
        FrameBuffer& fb,
        int x,
        int y,
        std::string_view latex_text,
        Color text_color = Color::White,
        Color bg_color = Color(18, 22, 34, 230)
    ) const;
};

} // namespace formulaic
