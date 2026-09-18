#include <Formulaic/hooks/pipeline_hooks.hpp>
#include <Formulaic/parser/expression.hpp>
#include <Formulaic/render/color.hpp>
#include <Formulaic/render/raster_engine.hpp>
#include <Formulaic/render/window_renderer.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>

int main() {
    std::cout << "====================================================================\n";
    std::cout << " Formulaic Dynamic Link (.dll) Real-time Interactive Visualizer     \n";
    std::cout << " Features: Anti-Aliased Rendering, Hover Highlighting & Event Hooks\n";
    std::cout << "====================================================================\n";

    auto renderer = formulaic::create_window_renderer();
    formulaic::RasterEngine engine;

    // 1. Compile mathematical expressions
    auto wave_expr = formulaic::Expression::parse("2.5 * sin(x - 1.5 * t)", {"x", "t"});
    auto cos_expr = formulaic::Expression::parse("1.8 * cos(0.8 * x + t)", {"x", "t"});
    auto implicit_expr = formulaic::Expression::parse("x^2 + y^2 - (12 + 4 * sin(2 * t))", {"x", "y", "t"});

    if (!wave_expr || !cos_expr || !implicit_expr) {
        std::cerr << "Failed to parse formulas!\n";
        return 1;
    }

    // 2. Register Interactive Event Callbacks
    std::string last_hovered_name;
    renderer->set_hover_callback([&](const formulaic::HoverInfo& info) {
        if (info.is_hovered) {
            if (info.target_name != last_hovered_name) {
                last_hovered_name = info.target_name;
                std::cout << "\n>>> [HOVER ENTER EVENT] Target: " << info.target_name
                          << " | World Pos: (" << std::fixed << std::setprecision(3)
                          << info.world_pos.x << ", " << info.world_pos.y << ")"
                          << " | Screen Pos: (" << info.screen_pos.x << ", " << info.screen_pos.y << ")\n";
            }
        } else {
            if (!last_hovered_name.empty()) {
                std::cout << "\n<<< [HOVER LEAVE EVENT] Exited: " << last_hovered_name << "\n";
                last_hovered_name.clear();
            }
        }
    });

    renderer->set_mouse_callback([](const formulaic::MouseEvent& me) {
        if (me.type == formulaic::MouseEventType::Down) {
            std::cout << "[MOUSE CLICK EVENT] Button: "
                      << (me.button == formulaic::MouseButton::Left ? "Left" : "Right")
                      << " at Screen (" << me.screen_pos.x << ", " << me.screen_pos.y << ")"
                      << " World (" << std::fixed << std::setprecision(2)
                      << me.world_pos.x << ", " << me.world_pos.y << ")\n";
        }
    });

    // 3. Configure HUD via PostRender Hook
    renderer->hooks().add_post_render_hook([&](formulaic::FrameBuffer& fb, const formulaic::Viewport& vp, double t) {
        // Draw elegant translucent glass card for instructions and live telemetry
        constexpr int card_w = 420;
        constexpr int card_h = 100;
        fb.fill_rounded_rect(12, 12, card_w, card_h, 8, formulaic::Color(18, 20, 28, 220));
        fb.draw_rounded_rect(12, 12, card_w, card_h, 8, formulaic::Color(60, 65, 80, 200), 1);

        fb.draw_text(24, 22, "Formulaic Interactive Math Engine (Anti-Aliased)", formulaic::Color::White);
        fb.draw_text(24, 40, "- Hover over any curve: Thicken line & inspect value", formulaic::Color::NeonGreen);
        fb.draw_text(24, 56, "- Left Drag: Pan world space | Scroll Wheel: Zoom", formulaic::Color::LightGray);
        fb.draw_text(24, 72, "- Double Click: Reset viewport to default bounds", formulaic::Color::LightGray);

        // Display live cursor position
        formulaic::Point2I mpos = renderer->mouse_position();
        if (mpos.x >= 0 && mpos.y >= 0) {
            formulaic::Point2D wpos = vp.screen_to_world(mpos.x, mpos.y);
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2)
                << "Cursor: Screen(" << mpos.x << "," << mpos.y << ") World(" << wpos.x << "," << wpos.y << ")";
            fb.draw_text(24, 88, oss.str(), formulaic::Color(160, 175, 200));
        }
    });

    // 4. Set Render Callback (Executed Every Frame)
    renderer->set_render_callback([&](formulaic::FrameBuffer& fb, const formulaic::Viewport& vp, double time_t) {
        fb.clear(formulaic::Color::BackgroundDark);
        engine.render_grid(fb, vp);

        // Hit-test mouse against both curves
        formulaic::Point2I mouse = renderer->mouse_position();
        formulaic::HitTestResult hit_wave = engine.hit_test_explicit(
            vp, wave_expr.value(), mouse, 15.0, time_t, "Wave: 2.5*sin(x-1.5t)"
        );
        formulaic::HitTestResult hit_cos = engine.hit_test_explicit(
            vp, cos_expr.value(), mouse, 15.0, time_t, "Cosine: 1.8*cos(0.8x+t)"
        );

        formulaic::HitTestResult active_hit;
        bool wave_hovered = false;
        bool cos_hovered = false;

        if (hit_wave.hit && (!hit_cos.hit || hit_wave.distance_screen_px <= hit_cos.distance_screen_px)) {
            active_hit = hit_wave;
            wave_hovered = true;
        } else if (hit_cos.hit) {
            active_hit = hit_cos;
            cos_hovered = true;
        }

        // Notify renderer of current hover status (fires hover callback on state changes)
        renderer->notify_hover(active_hit.to_hover_info());

        // Plot implicit circle
        engine.plot_implicit(fb, vp, implicit_expr.value(), formulaic::Color::NeonPink, 2.0, time_t);

        // Plot Wave curve: Thicken and brighten when hovered
        if (wave_hovered) {
            engine.plot_explicit(fb, vp, wave_expr.value(), formulaic::Color(120, 240, 255), 4.8, time_t);
        } else {
            engine.plot_explicit(fb, vp, wave_expr.value(), formulaic::Color::NeonBlue, 2.0, time_t);
        }

        // Plot Cosine curve: Thicken and brighten when hovered
        if (cos_hovered) {
            engine.plot_explicit(fb, vp, cos_expr.value(), formulaic::Color(255, 240, 120), 4.8, time_t);
        } else {
            engine.plot_explicit(fb, vp, cos_expr.value(), formulaic::Color::Yellow, 2.0, time_t);
        }

        // Render hover indicator (glowing snap point, axis projections, floating badge)
        if (active_hit.hit) {
            formulaic::Color highlight = wave_hovered ? formulaic::Color::NeonBlue : formulaic::Color::Yellow;
            engine.render_hover_indicator(fb, vp, active_hit, highlight);
        }
    });

#if defined(_WIN32)
    formulaic::Win32WindowDesc desc;
    desc.title = "Formulaic Real-Time Interactive Visualizer (Smooth Anti-Aliased Curves & Hover Events)";
    desc.width = 1080;
    desc.height = 720;
    desc.show = true;

    formulaic::Win32WindowHandle win_handle = formulaic::create_win32_window(desc, renderer.get());
    std::cout << "Interactive HWND window created successfully! Handle: " << win_handle.hwnd << "\n";
    std::cout << "Starting real-time 60 FPS animation and event loop...\n";
    std::cout << "Move mouse near curves to see dynamic thickening, value badge, and event output!\n";

    // Run interactive message loop
    formulaic::run_win32_message_loop(renderer.get(), true);

    formulaic::destroy_win32_window(win_handle);
#else
    std::cout << "Headless test run on non-Windows environment...\n";
    renderer->render(1.0);
    renderer->present();
#endif

    std::cout << "Interactive visualizer closed cleanly.\n";
    return 0;
}
