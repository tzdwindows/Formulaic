#include <Formulaic/parser/expression.hpp>
#include <Formulaic/render/color.hpp>
#include <Formulaic/render/frame_stream.hpp>
#include <Formulaic/render/framebuffer.hpp>
#include <Formulaic/render/image_export.hpp>
#include <Formulaic/render/raster_engine.hpp>
#include <Formulaic/render/viewport.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: [" << #cond << "] " << (msg) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "=========================================\n";
    std::cout << " Running Offline Raster & Exporter Tests \n";
    std::cout << "=========================================\n";

    const std::filesystem::path output_dir = "test_output";
    std::filesystem::create_directories(output_dir);

    // 1. FrameBuffer basic operations & pixel manipulation
    {
        std::cout << "[Test 1] FrameBuffer Primitives & Blending... ";
        formulaic::FrameBuffer fb(200, 150, formulaic::Color::Black);
        TEST_ASSERT(fb.width() == 200, "Width match");
        TEST_ASSERT(fb.height() == 150, "Height match");

        fb.set_pixel(10, 20, formulaic::Color::Red);
        auto c = fb.get_pixel(10, 20);
        TEST_ASSERT(c.r == formulaic::Color::Red.r && c.g == formulaic::Color::Red.g && c.b == formulaic::Color::Red.b, "Pixel readback");

        // Alpha blending test
        fb.blend_pixel(10, 20, formulaic::Color(0, 0, 255, 128), formulaic::BlendMode::AlphaBlend);
        auto blended = fb.get_pixel(10, 20);
        TEST_ASSERT(blended.r > 0 && blended.b > 0, "Blended red and blue channels");

        // Lines and shapes
        fb.draw_line(0, 0, 199, 149, formulaic::Color::White);
        fb.draw_circle(100, 75, 30, formulaic::Color::Green);
        fb.fill_circle(100, 75, 10, formulaic::Color::Yellow);
        fb.draw_text(10, 10, "Formulaic Engine", formulaic::Color::Cyan);

        std::cout << "PASSED\n";
    }

    // 2. Explicit 1D Function Rasterization y = sin(x) * x
    {
        std::cout << "[Test 2] Explicit 1D Plotting & BMP Export... ";
        formulaic::FrameBuffer fb(800, 600, formulaic::Color::BackgroundDark);
        formulaic::Viewport vp(800, 600, formulaic::Rect2D(-10.0, 10.0, -10.0, 10.0));
        formulaic::RasterEngine engine;

        engine.render_grid(fb, vp);

        auto expr = formulaic::Expression::parse("sin(x) * x", {"x"});
        TEST_ASSERT(expr.has_value(), expr.error().format());
        engine.plot_explicit(fb, vp, expr.value(), formulaic::Color::NeonBlue, 2);

        auto bmp_path = output_dir / "test_explicit_sin_x.bmp";
        auto res = formulaic::ImageExport::save_bmp(fb, bmp_path);
        TEST_ASSERT(res.has_value(), res.error().format());
        TEST_ASSERT(std::filesystem::exists(bmp_path), "BMP file must exist");
        TEST_ASSERT(std::filesystem::file_size(bmp_path) > 1000, "BMP file must have non-trivial size");

        std::cout << "PASSED -> Saved to " << bmp_path.string() << "\n";
    }

    // 3. Parametric 2D Curve Rasterization (Lissajous: x = sin(3t), y = sin(4t))
    {
        std::cout << "[Test 3] Parametric 2D Curve Plotting... ";
        formulaic::FrameBuffer fb(800, 600, formulaic::Color::BackgroundDark);
        formulaic::Viewport vp(800, 600, formulaic::Rect2D(-2.0, 2.0, -2.0, 2.0));
        formulaic::RasterEngine engine;

        engine.render_grid(fb, vp);

        auto expr_x = formulaic::Expression::parse("sin(3 * t)", {"t"});
        auto expr_y = formulaic::Expression::parse("sin(4 * t)", {"t"});
        TEST_ASSERT(expr_x.has_value() && expr_y.has_value(), "Parametric expressions parsed");

        engine.plot_parametric(fb, vp, expr_x.value(), expr_y.value(), 0.0, 6.283185307, 2000,
                               formulaic::Color::NeonPink, 2);

        auto bmp_path = output_dir / "test_parametric_lissajous.bmp";
        auto res = formulaic::ImageExport::save_bmp(fb, bmp_path);
        TEST_ASSERT(res.has_value(), res.error().format());
        TEST_ASSERT(std::filesystem::exists(bmp_path), "Parametric BMP exists");

        std::cout << "PASSED -> Saved to " << bmp_path.string() << "\n";
    }

    // 4. Implicit 2D Function Rasterization f(x, y) = x^2 + y^2 - 25 = 0 (Circle R=5)
    {
        std::cout << "[Test 4] Implicit Function Marching Squares... ";
        formulaic::FrameBuffer fb(800, 600, formulaic::Color::BackgroundDark);
        formulaic::Viewport vp(800, 600, formulaic::Rect2D(-8.0, 8.0, -8.0, 8.0));
        formulaic::RasterEngine engine;

        engine.render_grid(fb, vp);

        auto expr = formulaic::Expression::parse("x^2 + y^2 - 25", {"x", "y"});
        TEST_ASSERT(expr.has_value(), expr.error().format());
        engine.plot_implicit(fb, vp, expr.value(), formulaic::Color::NeonGreen, 2);

        auto bmp_path = output_dir / "test_implicit_circle.bmp";
        auto res = formulaic::ImageExport::save_bmp(fb, bmp_path);
        TEST_ASSERT(res.has_value(), res.error().format());
        TEST_ASSERT(std::filesystem::exists(bmp_path), "Implicit BMP exists");

        std::cout << "PASSED -> Saved to " << bmp_path.string() << "\n";
    }

    // 4b. Implicit 2D Asymptote / Pole Rejection Test (1/x + 1/y = 0)
    {
        std::cout << "[Test 4b] Implicit Function Asymptote & Pole Rejection (1/x + 1/y = 0)... ";
        formulaic::FrameBuffer fb(800, 600, formulaic::Color::BackgroundDark);
        formulaic::Viewport vp(800, 600, formulaic::Rect2D(-3.0, 3.0, -3.0, 3.0));
        formulaic::RasterEngine engine;

        auto expr = formulaic::Expression::parse_equation("1/x + 1/y = 0");
        TEST_ASSERT(expr.has_value(), expr.error().format());
        engine.plot_implicit(fb, vp, expr.value(), formulaic::Color::NeonPink, 2);

        // Check that pixels on the positive x axis (x = 2.0, y = 0.0) are NOT drawn in pink
        const int test_x = 666;
        const int test_y = 300;
        formulaic::Color col_axis = fb.get_pixel(test_x, test_y);
        TEST_ASSERT(col_axis == formulaic::Color::BackgroundDark, "Positive x axis must not contain spurious pole line");

        // Check that positive y axis (x = 0.0, y = 2.0) is NOT drawn in pink
        formulaic::Color col_y_axis = fb.get_pixel(400, 100);
        TEST_ASSERT(col_y_axis == formulaic::Color::BackgroundDark, "Positive y axis must not contain spurious pole line");

        // Check that the true curve point on y = -x (e.g. x = -1.5, y = 1.5) IS drawn
        bool found_diag_pixel = false;
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                if (fb.get_pixel(200 + dx, 150 + dy) != formulaic::Color::BackgroundDark) {
                    found_diag_pixel = true;
                    break;
                }
            }
            if (found_diag_pixel) break;
        }
        TEST_ASSERT(found_diag_pixel, "True diagonal curve y = -x must be rendered");

        auto bmp_path = output_dir / "test_pole_rejection_1_x_1_y.bmp";
        auto res = formulaic::ImageExport::save_bmp(fb, bmp_path);
        TEST_ASSERT(res.has_value(), res.error().format());
        std::cout << "PASSED -> Saved to " << bmp_path.string() << "\n";
    }

    // 5. Scalar Field 2D Heatmap
    {
        std::cout << "[Test 5] Scalar Field Heatmap Rasterization... ";
        formulaic::FrameBuffer fb(400, 300, formulaic::Color::BackgroundDark);
        formulaic::Viewport vp(400, 300, formulaic::Rect2D(-4.0, 4.0, -3.0, 3.0));
        formulaic::RasterEngine engine;

        auto expr = formulaic::Expression::parse("sin(x) * cos(y)", {"x", "y"});
        TEST_ASSERT(expr.has_value(), expr.error().format());
        engine.plot_scalar_field(fb, vp, expr.value(), formulaic::ColormapType::Viridis, -1.0, 1.0);

        auto bmp_path = output_dir / "test_scalar_field.bmp";
        auto res = formulaic::ImageExport::save_bmp(fb, bmp_path);
        TEST_ASSERT(res.has_value(), res.error().format());

        // Test raw RGBA export
        auto raw_path = output_dir / "test_scalar_field.raw";
        auto raw_res = formulaic::ImageExport::save_raw_rgba(fb, raw_path);
        TEST_ASSERT(raw_res.has_value(), raw_res.error().format());
        TEST_ASSERT(std::filesystem::file_size(raw_path) == 400 * 300 * 4, "Raw size matches 400x300x4");

        std::cout << "PASSED -> Saved to " << bmp_path.string() << "\n";
    }

    // 6. FrameStream Multi-frame Animation Generator
    {
        std::cout << "[Test 6] FrameStream Time-step Animation Sequence... ";
        formulaic::FrameStream stream(400, 300, 0.0, 0.2, 20.0); // 5 frames
        TEST_ASSERT(stream.total_frames() == 5, "Total frame count == 5");

        auto expr = formulaic::Expression::parse("sin(x + 5 * t)", {"x", "t"});
        TEST_ASSERT(expr.has_value(), expr.error().format());
        formulaic::RasterEngine engine;
        formulaic::Viewport vp(400, 300, formulaic::Rect2D(-5.0, 5.0, -2.0, 2.0));

        size_t frames_rendered = 0;
        stream.generate(
            [&](formulaic::FrameBuffer& fb, double time, size_t idx) {
                fb.clear(formulaic::Color::BackgroundDark);
                engine.render_grid(fb, vp);
                engine.plot_explicit(fb, vp, expr.value(), formulaic::Color::NeonPink, 2, time);
            },
            [&](const formulaic::Frame& frame) -> bool {
                frames_rendered++;
                return true;
            }
        );
        TEST_ASSERT(frames_rendered == 5, "Rendered exactly 5 frames");

        // Dump to directory
        auto dump_res = stream.dump_to_directory(
            output_dir / "stream_frames",
            "frame",
            [&](formulaic::FrameBuffer& fb, double time, size_t idx) {
                fb.clear(formulaic::Color::BackgroundDark);
                engine.render_grid(fb, vp);
                engine.plot_explicit(fb, vp, expr.value(), formulaic::Color::NeonBlue, 2, time);
            }
        );
        TEST_ASSERT(dump_res.has_value() && dump_res.value() == 5, "Dumped 5 frames to disk");
        std::cout << "PASSED (5 continuous frames generated)\n";
    }

    // 7. Sub-pixel Anti-Aliased Line and Geometric Rendering
    {
        std::cout << "[Test 7] Sub-pixel Anti-Aliasing (Distance-field coverage)... ";
        formulaic::FrameBuffer fb(100, 100, formulaic::Color::Black);

        // Draw an anti-aliased line diagonally
        fb.draw_line_aa(10.0, 10.0, 80.0, 50.0, formulaic::Color::White, 2.5);

        // Check that pixels near the center of the line are bright, and edges have fractional anti-aliasing
        auto center_px = fb.get_pixel(45, 30);
        TEST_ASSERT(center_px.r > 200, "Core pixel of anti-aliased line is bright");

        // Verify fractional coverage exists along edge (anti-aliasing smooth transition)
        bool found_fractional_pixel = false;
        for (int y = 25; y <= 35; ++y) {
            for (int x = 40; x <= 50; ++x) {
                auto px = fb.get_pixel(x, y);
                if (px.r > 20 && px.r < 235) {
                    found_fractional_pixel = true;
                    break;
                }
            }
            if (found_fractional_pixel) break;
        }
        TEST_ASSERT(found_fractional_pixel, "Anti-aliased line has smooth sub-pixel alpha coverage");

        // Test dashed line AA and circles AA
        fb.draw_dashed_line_aa(5.0, 5.0, 95.0, 5.0, formulaic::Color::Cyan, 2.0, 5.0, 3.0);
        fb.fill_circle_aa(50.0, 50.0, 15.0, formulaic::Color::NeonGreen);
        fb.draw_circle_aa(50.0, 50.0, 15.0, formulaic::Color::White, 1.5);
        fb.fill_rounded_rect(10, 60, 40, 30, 4, formulaic::Color::NeonPink);

        auto bmp_path = output_dir / "test_antialiasing_primitives.bmp";
        auto res = formulaic::ImageExport::save_bmp(fb, bmp_path);
        TEST_ASSERT(res.has_value(), res.error().format());
        std::cout << "PASSED -> Saved to " << bmp_path.string() << "\n";
    }

    // 8. Curve Hit-Testing and Interactive Hover Rendering
    {
        std::cout << "[Test 8] Curve Hit-Testing & Hover Indicator... ";
        formulaic::FrameBuffer fb(800, 600, formulaic::Color::BackgroundDark);
        formulaic::Viewport vp(800, 600, formulaic::Rect2D(-5.0, 5.0, -5.0, 5.0));
        formulaic::RasterEngine engine;

        auto expr = formulaic::Expression::parse("x^2 - 2", {"x"});
        TEST_ASSERT(expr.has_value(), expr.error().format());

        // Known point on curve: x = 2.0, y = 2.0
        formulaic::Point2I screen_pt = vp.world_to_screen({2.0, 2.0});

        // Hit test directly at the point
        auto hit_exact = engine.hit_test_explicit(vp, expr.value(), screen_pt, 10.0, 0.0, "Parabola");
        TEST_ASSERT(hit_exact.hit, "Hit test detected known point on curve");
        TEST_ASSERT(std::abs(hit_exact.world_pos.x - 2.0) < 0.05, "Hit world X matches");
        TEST_ASSERT(std::abs(hit_exact.world_pos.y - 2.0) < 0.05, "Hit world Y matches");

        // Hit test near the point within tolerance (e.g. 5px away)
        formulaic::Point2I screen_near = {screen_pt.x + 3, screen_pt.y + 4};
        auto hit_near = engine.hit_test_explicit(vp, expr.value(), screen_near, 12.0, 0.0, "Parabola");
        TEST_ASSERT(hit_near.hit, "Hit test detected point within tolerance");

        // Hit test far from curve (e.g. 100px away)
        formulaic::Point2I screen_far = {screen_pt.x, screen_pt.y + 100};
        auto hit_far = engine.hit_test_explicit(vp, expr.value(), screen_far, 12.0, 0.0, "Parabola");
        TEST_ASSERT(!hit_far.hit, "Hit test rejects points far from curve");

        // Render hover indicator onto framebuffer
        engine.plot_explicit(fb, vp, expr.value(), formulaic::Color::NeonBlue, 2.0);
        engine.render_hover_indicator(fb, vp, hit_exact, formulaic::Color::NeonGreen);

        auto bmp_path = output_dir / "test_hover_indicator.bmp";
        auto res = formulaic::ImageExport::save_bmp(fb, bmp_path);
        TEST_ASSERT(res.has_value(), res.error().format());
        std::cout << "PASSED -> Saved to " << bmp_path.string() << "\n";
    }

    std::cout << "\n>>> All Offline Raster & Exporter Tests PASSED successfully! <<<\n";
    return 0;
}
