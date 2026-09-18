#include <Formulaic/hooks/pipeline_hooks.hpp>
#include <Formulaic/parser/expression.hpp>
#include <Formulaic/render/color.hpp>
#include <Formulaic/render/image_export.hpp>
#include <Formulaic/render/raster_engine.hpp>
#include <Formulaic/render/viewport.hpp>
#include <iostream>

int main() {
    std::cout << "========================================================\n";
    std::cout << " Formulaic Static Library Linkage & Rendering Demo      \n";
    std::cout << "========================================================\n";

    // 1. Setup high-resolution framebuffer and viewport
    constexpr int width = 1280;
    constexpr int height = 720;
    formulaic::FrameBuffer fb(width, height, formulaic::Color::BackgroundDark);
    formulaic::Viewport vp(width, height, formulaic::Rect2D(-12.0, 12.0, -8.0, 8.0));
    formulaic::RasterEngine engine;

    // 2. Setup rendering pipeline hooks
    formulaic::PipelineHooks hooks;

    // Pre-render hook: custom title and background watermark
    hooks.add_pre_render_hook([](formulaic::FrameBuffer& buffer, const formulaic::Viewport&, double) {
        buffer.draw_text(20, 20, "Formulaic High Performance Math Renderer", formulaic::Color::LightGray, 2);
    });

    // Post-render hook: visual legend and statistics
    hooks.add_post_render_hook([](formulaic::FrameBuffer& buffer, const formulaic::Viewport&, double) {
        buffer.draw_rect(width - 320, 20, 300, 110, formulaic::Color::GridGray);
        buffer.fill_rect(width - 319, 21, 298, 108, formulaic::Color(25, 25, 32, 220));

        buffer.draw_text(width - 305, 35, "Legend:", formulaic::Color::White);
        buffer.draw_line(width - 305, 55, width - 275, 55, formulaic::Color::NeonBlue, 3);
        buffer.draw_text(width - 265, 50, "y = sin(x) * (x / 2)", formulaic::Color::NeonBlue);

        buffer.draw_line(width - 305, 75, width - 275, 75, formulaic::Color::NeonPink, 3);
        buffer.draw_text(width - 265, 70, "x^2 + y^2 = 25", formulaic::Color::NeonPink);

        buffer.draw_line(width - 305, 95, width - 275, 95, formulaic::Color::NeonGreen, 3);
        buffer.draw_text(width - 265, 90, "Parametric Butterfly", formulaic::Color::NeonGreen);
    });

    // Pixel shader hook: dynamic glowing gradient
    hooks.set_pixel_shader_hook([](double wx, double wy, double val, const formulaic::Color& base, double time) {
        // Color modulated by distance from origin
        double dist = std::sqrt(wx * wx + wy * wy);
        double factor = 0.5 + 0.5 * std::sin(dist * 0.8);
        return formulaic::Color::lerp(base, formulaic::Color::White, factor * 0.3);
    });

    // 3. Render coordinate grid and axes
    hooks.execute_pre_render(fb, vp, 0.0);
    engine.render_grid(fb, vp);

    // 4. Plot explicit 1D curve: y = sin(x) * (x / 2)
    auto expr1 = formulaic::Expression::parse("sin(x) * (x / 2)", {"x"});
    if (expr1) {
        std::cout << "Compiling explicit formula: " << expr1->source() << "\n";
        engine.plot_explicit(fb, vp, expr1.value(), formulaic::Color::NeonBlue, 2, 0.0, &hooks);
    }

    // 5. Plot implicit 2D circle: x^2 + y^2 - 25 = 0
    auto expr2 = formulaic::Expression::parse("x^2 + y^2 - 25", {"x", "y"});
    if (expr2) {
        std::cout << "Compiling implicit formula: " << expr2->source() << "\n";
        engine.plot_implicit(fb, vp, expr2.value(), formulaic::Color::NeonPink, 2, 0.0, &hooks);
    }

    // 6. Plot parametric curve
    auto expr_px = formulaic::Expression::parse("sin(t) * (exp(cos(t)) - 2 * cos(4 * t))", {"t"});
    auto expr_py = formulaic::Expression::parse("cos(t) * (exp(cos(t)) - 2 * cos(4 * t))", {"t"});
    if (expr_px && expr_py) {
        std::cout << "Compiling parametric formula...\n";
        engine.plot_parametric(fb, vp, expr_px.value(), expr_py.value(), 0.0, 6.283185307, 3000,
                               formulaic::Color::NeonGreen, 2, 0.0, &hooks);
    }

    // Execute post render hooks (overlays & HUD)
    hooks.execute_post_render(fb, vp, 0.0);

    // 7. Save output image to disk
    const std::string out_file = "static_demo_output.bmp";
    auto save_res = formulaic::ImageExport::save_bmp(fb, out_file);
    if (save_res) {
        std::cout << "\nSuccessfully rendered and exported demo image to: " << out_file << "\n";
    } else {
        std::cerr << "Failed to export image: " << save_res.error().format() << "\n";
    }

    return 0;
}
