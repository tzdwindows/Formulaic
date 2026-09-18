#include <Formulaic/hooks/pipeline_hooks.hpp>
#include <Formulaic/parser/expression.hpp>
#include <Formulaic/render/color.hpp>
#include <Formulaic/render/raster_engine.hpp>
#include <Formulaic/render/window_renderer.hpp>
#include <cassert>
#include <cmath>
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
    std::cout << "===========================================\n";
    std::cout << " Running Window Binding & Hook Tests       \n";
    std::cout << "===========================================\n";

    // 1. Pipeline Hooks Unit Verification
    {
        std::cout << "[Test 1] Pipeline Hooks Lifecycle & Invocation... ";
        formulaic::PipelineHooks hooks;

        bool pre_called = false;
        bool post_called = false;
        bool shader_called = false;
        bool transform_called = false;

        hooks.add_pre_render_hook([&](formulaic::FrameBuffer& fb, const formulaic::Viewport& vp, double t) {
            pre_called = true;
            fb.clear(formulaic::Color::DarkGray);
        });

        hooks.add_post_render_hook([&](formulaic::FrameBuffer& fb, const formulaic::Viewport& vp, double t) {
            post_called = true;
            fb.draw_text(10, 10, "HUD Overlay Active", formulaic::Color::Yellow);
        });

        hooks.set_coordinate_transform_hook([&](const formulaic::Point2D& pt, const formulaic::Viewport& vp) -> formulaic::Point2D {
            transform_called = true;
            // Warping test: y' = y + 0.5 * x
            return {pt.x, pt.y + 0.5 * pt.x};
        });

        hooks.set_pixel_shader_hook([&](double wx, double wy, double val, const formulaic::Color& base, double t) -> formulaic::Color {
            shader_called = true;
            return formulaic::Color(255, 128, 0, 255); // Custom amber shade
        });

        formulaic::FrameBuffer fb(400, 300);
        formulaic::Viewport vp(400, 300);
        formulaic::RasterEngine engine;

        // Execute pre-render
        hooks.execute_pre_render(fb, vp, 0.0);
        TEST_ASSERT(pre_called, "Pre-render hook executed");
        TEST_ASSERT(fb.get_pixel(0, 0).to_rgba32() == formulaic::Color::DarkGray.to_rgba32(), "Pre-render cleared buffer");

        // Rasterize function with hooks
        auto expr = formulaic::Expression::parse("sin(x)", {"x"});
        TEST_ASSERT(expr.has_value(), expr.error().format());
        engine.plot_explicit(fb, vp, expr.value(), formulaic::Color::NeonBlue, 2, 0.0, &hooks);
        TEST_ASSERT(transform_called, "Coordinate transform hook executed during rasterization");
        TEST_ASSERT(shader_called, "Pixel shader hook executed during rasterization");

        // Execute post-render
        hooks.execute_post_render(fb, vp, 0.0);
        TEST_ASSERT(post_called, "Post-render hook executed");

        std::cout << "PASSED\n";
    }

    // 2. Native Window Handle Binding & Double Buffering
    {
        std::cout << "[Test 2] Window Renderer HWND Binding & Double Buffering... ";
        auto renderer = formulaic::create_window_renderer();
        TEST_ASSERT(renderer != nullptr, "Created platform window renderer");
        TEST_ASSERT(!renderer->is_attached(), "Initially detached");

        bool render_cb_executed = false;
        bool hook_executed = false;

        renderer->hooks().add_post_render_hook([&](formulaic::FrameBuffer& fb, const formulaic::Viewport& vp, double t) {
            hook_executed = true;
        });

        renderer->set_render_callback([&](formulaic::FrameBuffer& fb, const formulaic::Viewport& vp, double t) {
            render_cb_executed = true;
            fb.draw_circle(200, 150, 40, formulaic::Color::NeonGreen);
        });

#if defined(_WIN32)
        // Create an offscreen/hidden test window to test native HWND attachment
        formulaic::Win32WindowDesc desc;
        desc.title = "Formulaic Test Window";
        desc.width = 640;
        desc.height = 480;
        desc.show = false; // Headless-friendly

        formulaic::Win32WindowHandle win_handle = formulaic::create_win32_window(desc, renderer.get());
        TEST_ASSERT(win_handle.hwnd != nullptr, "Created Win32 window HWND");
        TEST_ASSERT(renderer->is_attached(), "Renderer attached to HWND");
        TEST_ASSERT(renderer->native_handle() == win_handle.hwnd, "HWND pointer match");

        // Test resize logic
        renderer->on_resize(800, 600);
        TEST_ASSERT(renderer->framebuffer().width() == 800, "Framebuffer width after resize");
        TEST_ASSERT(renderer->framebuffer().height() == 600, "Framebuffer height after resize");
        TEST_ASSERT(renderer->viewport().width() == 800, "Viewport width after resize");

        // Test render and present
        renderer->render(1.0);
        TEST_ASSERT(render_cb_executed, "Render callback executed");
        TEST_ASSERT(hook_executed, "Post-render hook executed in window pipeline");

        renderer->present(); // Blits to window DC without error

        // Cleanup
        renderer->detach();
        TEST_ASSERT(!renderer->is_attached(), "Detached successfully");
        formulaic::destroy_win32_window(win_handle);
#else
        // Mock handle for non-Windows platforms
        int mock_handle = 12345;
        renderer->attach(&mock_handle);
        TEST_ASSERT(renderer->is_attached(), "Attached mock handle");
        renderer->render(1.0);
        renderer->present();
        renderer->detach();
#endif

        std::cout << "PASSED\n";
    }

    // 3. Interactive Events & Hover Callbacks
    {
        std::cout << "[Test 3] Interactive Mouse & Hover Event Callbacks... ";
        auto renderer = formulaic::create_window_renderer();

        bool mouse_event_received = false;
        formulaic::MouseEvent captured_me{};
        renderer->set_mouse_callback([&](const formulaic::MouseEvent& me) {
            mouse_event_received = true;
            captured_me = me;
        });

        bool hover_event_received = false;
        formulaic::HoverInfo captured_hover{};
        renderer->set_hover_callback([&](const formulaic::HoverInfo& hover) {
            hover_event_received = true;
            captured_hover = hover;
        });

        // Dispatch mouse event
        formulaic::MouseEvent me{};
        me.type = formulaic::MouseEventType::Move;
        me.screen_pos = {150, 280};
        me.world_pos = {1.5, -2.8};
        me.button = formulaic::MouseButton::None;
        renderer->dispatch_mouse_event(me);

        TEST_ASSERT(mouse_event_received, "Mouse callback executed on dispatch");
        TEST_ASSERT(captured_me.screen_pos.x == 150 && captured_me.screen_pos.y == 280, "Captured screen position matches");
        TEST_ASSERT(renderer->mouse_position().x == 150 && renderer->mouse_position().y == 280, "Renderer mouse_position matches");

        // Notify hover
        formulaic::HoverInfo hi{};
        hi.is_hovered = true;
        hi.target_name = "sin(x)";
        hi.world_pos = {1.5, 0.997};
        hi.screen_pos = {150, 120};
        hi.value = 0.997;
        hi.distance_px = 3.2;
        hi.detail_text = "sin(x): x=1.5, y=0.997";
        renderer->notify_hover(hi);

        TEST_ASSERT(hover_event_received, "Hover callback executed on notify_hover");
        TEST_ASSERT(captured_hover.is_hovered, "Hover state is hovered");
        TEST_ASSERT(captured_hover.target_name == "sin(x)", "Hover target name matches");
        TEST_ASSERT(renderer->current_hover().is_hovered, "Renderer current_hover matches");
        TEST_ASSERT(renderer->current_hover().target_name == "sin(x)", "Renderer current_hover target matches");

        std::cout << "PASSED\n";
    }

    std::cout << "\n>>> All Window Binding & Hook Tests PASSED successfully! <<<\n";
    return 0;
}
