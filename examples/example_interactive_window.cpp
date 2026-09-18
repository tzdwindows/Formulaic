#include <Formulaic/hooks/pipeline_hooks.hpp>
#include <Formulaic/parser/expression.hpp>
#include <Formulaic/render/color.hpp>
#include <Formulaic/render/raster_engine.hpp>
#include <Formulaic/render/window_renderer.hpp>
#include <iostream>

int main() {
    std::cout << "===========================================================\n";
    std::cout << " Formulaic Dynamic Link (.dll) Real-time Interactive Window \n";
    std::cout << "===========================================================\n";

    auto renderer = formulaic::create_window_renderer();
    formulaic::RasterEngine engine;

    // Compile dynamic animated mathematical expressions
    auto wave_expr = formulaic::Expression::parse("sin(x - 3 * t) * (1 + 0.3 * cos(t))", {"x", "t"});
    auto implicit_expr = formulaic::Expression::parse("x^2 + y^2 - (16 + 8 * sin(2 * t))", {"x", "y", "t"});

    if (!wave_expr || !implicit_expr) {
        std::cerr << "Failed to parse formulas!\n";
        return 1;
    }

    // Configure pipeline hooks
    renderer->hooks().add_post_render_hook([](formulaic::FrameBuffer& fb, const formulaic::Viewport& vp, double t) {
        // Render interactive HUD
        fb.draw_rect(10, 10, 380, 75, formulaic::Color::GridGray);
        fb.fill_rect(11, 11, 378, 73, formulaic::Color(20, 20, 28, 220));

        fb.draw_text(20, 20, "Controls:", formulaic::Color::White);
        fb.draw_text(20, 38, "- Left Mouse Drag: Pan world space", formulaic::Color::LightGray);
        fb.draw_text(20, 52, "- Mouse Wheel: Zoom in / out", formulaic::Color::LightGray);
        fb.draw_text(20, 66, "- Double Click: Reset view", formulaic::Color::LightGray);
    });

    // Set interactive render callback executed every frame
    renderer->set_render_callback([&](formulaic::FrameBuffer& fb, const formulaic::Viewport& vp, double time_t) {
        fb.clear(formulaic::Color::BackgroundDark);
        engine.render_grid(fb, vp);

        // Plot animated wave
        engine.plot_explicit(fb, vp, wave_expr.value(), formulaic::Color::NeonBlue, 2, time_t);

        // Plot pulsating implicit circle
        engine.plot_implicit(fb, vp, implicit_expr.value(), formulaic::Color::NeonPink, 2, time_t);
    });

#if defined(_WIN32)
    formulaic::Win32WindowDesc desc;
    desc.title = "Formulaic Real-time Interactive Visualizer (HWND Double Buffered)";
    desc.width = 1024;
    desc.height = 680;
    desc.show = true;

    formulaic::Win32WindowHandle win_handle = formulaic::create_win32_window(desc, renderer.get());
    std::cout << "Window created successfully! Handle: " << win_handle.hwnd << "\n";
    std::cout << "Running real-time message loop with 60 FPS double-buffered rendering...\n";

    // Run interactive message loop with continuous animation
    formulaic::run_win32_message_loop(renderer.get(), true);

    formulaic::destroy_win32_window(win_handle);
#else
    std::cout << "Non-Windows platform: executing headless test frame...\n";
    renderer->render(1.0);
    renderer->present();
#endif

    std::cout << "Interactive session ended cleanly.\n";
    return 0;
}
