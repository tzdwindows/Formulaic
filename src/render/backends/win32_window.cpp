#include <Formulaic/render/window_renderer.hpp>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <chrono>

namespace formulaic {

class Win32WindowRenderer final : public IWindowRenderer {
public:
    Win32WindowRenderer() : framebuffer_(800, 600), viewport_(800, 600) {
        init_bmi(800, 600);
    }

    ~Win32WindowRenderer() override {
        detach();
    }

    bool attach(void* native_handle) override {
        detach();
        if (!native_handle) return false;

        hwnd_ = reinterpret_cast<HWND>(native_handle);
        hdc_ = GetDC(hwnd_);
        if (!hdc_) return false;

        RECT rc;
        GetClientRect(hwnd_, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        if (w > 0 && h > 0) {
            on_resize(w, h);
        }

        attached_ = true;
        return true;
    }

    void detach() override {
        if (hwnd_ && hdc_) {
            ReleaseDC(hwnd_, hdc_);
            hdc_ = nullptr;
        }
        hwnd_ = nullptr;
        attached_ = false;
    }

    void on_resize(int new_width, int new_height) override {
        if (new_width <= 0 || new_height <= 0) return;
        framebuffer_.resize(new_width, new_height);
        viewport_.set_size(new_width, new_height);
        init_bmi(new_width, new_height);
    }

    void render(double time_t) override {
        hooks_.execute_pre_render(framebuffer_, viewport_, time_t);
        if (render_callback_) {
            render_callback_(framebuffer_, viewport_, time_t);
        }
        hooks_.execute_post_render(framebuffer_, viewport_, time_t);
    }

    void present() override {
        if (!hwnd_ || !hdc_) return;

        // Blit internal 32-bit BGRA surface directly to window DC
        StretchDIBits(
            hdc_,
            0, 0, framebuffer_.width(), framebuffer_.height(),
            0, 0, framebuffer_.width(), framebuffer_.height(),
            framebuffer_.data(),
            &bmi_,
            DIB_RGB_COLORS,
            SRCCOPY
        );
    }

    void set_render_callback(RenderCallback callback) override {
        render_callback_ = std::move(callback);
    }

    void set_mouse_callback(MouseCallback callback) override {
        mouse_callback_ = std::move(callback);
    }

    void set_hover_callback(HoverCallback callback) override {
        hover_callback_ = std::move(callback);
    }

    void dispatch_mouse_event(const MouseEvent& event) override {
        mouse_pos_ = event.screen_pos;
        if (mouse_callback_) {
            mouse_callback_(event);
        }
    }

    void notify_hover(const HoverInfo& hover) override {
        current_hover_ = hover;
        if (hover_callback_) {
            hover_callback_(hover);
        }
    }

    [[nodiscard]] Point2I mouse_position() const noexcept override {
        return mouse_pos_;
    }

    [[nodiscard]] const HoverInfo& current_hover() const noexcept override {
        return current_hover_;
    }

    [[nodiscard]] FrameBuffer& framebuffer() noexcept override { return framebuffer_; }
    [[nodiscard]] const FrameBuffer& framebuffer() const noexcept override { return framebuffer_; }
    [[nodiscard]] Viewport& viewport() noexcept override { return viewport_; }
    [[nodiscard]] const Viewport& viewport() const noexcept override { return viewport_; }
    [[nodiscard]] PipelineHooks& hooks() noexcept override { return hooks_; }
    [[nodiscard]] const PipelineHooks& hooks() const noexcept override { return hooks_; }

    [[nodiscard]] void* native_handle() const noexcept override { return reinterpret_cast<void*>(hwnd_); }
    [[nodiscard]] bool is_attached() const noexcept override { return attached_; }

private:
    void init_bmi(int w, int h) {
        ZeroMemory(&bmi_, sizeof(bmi_));
        bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi_.bmiHeader.biWidth = w;
        bmi_.bmiHeader.biHeight = -h; // Negative height = top-down DIB, perfectly aligned
        bmi_.bmiHeader.biPlanes = 1;
        bmi_.bmiHeader.biBitCount = 32;
        bmi_.bmiHeader.biCompression = BI_RGB;
    }

    HWND hwnd_{nullptr};
    HDC hdc_{nullptr};
    bool attached_{false};

    FrameBuffer framebuffer_;
    Viewport viewport_;
    PipelineHooks hooks_;
    RenderCallback render_callback_;
    Point2I mouse_pos_{-1, -1};
    HoverInfo current_hover_{};
    MouseCallback mouse_callback_;
    HoverCallback hover_callback_;
    BITMAPINFO bmi_{};
};

std::unique_ptr<IWindowRenderer> create_window_renderer() {
    return std::make_unique<Win32WindowRenderer>();
}

namespace {

struct WindowState {
    IWindowRenderer* renderer{nullptr};
    bool is_dragging{false};
    int last_mouse_x{0};
    int last_mouse_y{0};
    bool is_animated{false};
    std::chrono::high_resolution_clock::time_point start_time{std::chrono::high_resolution_clock::now()};

    [[nodiscard]] double get_elapsed_time() const noexcept {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(now - start_time).count();
    }
};

LRESULT CALLBACK ManagedWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    WindowState* state = reinterpret_cast<WindowState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            auto* create_struct = reinterpret_cast<CREATESTRUCT*>(lparam);
            state = reinterpret_cast<WindowState*>(create_struct->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
            if (state && state->renderer) {
                state->renderer->attach(hwnd);
            }
            return 0;
        }

        case WM_SIZE: {
            int w = LOWORD(lparam);
            int h = HIWORD(lparam);
            if (state && state->renderer && w > 0 && h > 0) {
                state->renderer->on_resize(w, h);
                state->renderer->render(state->get_elapsed_time());
                state->renderer->present();
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1; // Prevent flickering

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            if (state && state->renderer) {
                state->renderer->present();
            }
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            if (state && state->renderer) {
                state->is_dragging = true;
                state->last_mouse_x = GET_X_LPARAM(lparam);
                state->last_mouse_y = GET_Y_LPARAM(lparam);
                SetCapture(hwnd);

                MouseEvent me{};
                me.type = MouseEventType::Down;
                me.button = MouseButton::Left;
                me.screen_pos = {state->last_mouse_x, state->last_mouse_y};
                me.world_pos = state->renderer->viewport().screen_to_world(state->last_mouse_x, state->last_mouse_y);
                me.ctrl_down = (wparam & MK_CONTROL) != 0;
                me.shift_down = (wparam & MK_SHIFT) != 0;
                state->renderer->dispatch_mouse_event(me);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            if (state && state->renderer) {
                if (state->is_dragging) {
                    state->is_dragging = false;
                    ReleaseCapture();
                }
                int mx = GET_X_LPARAM(lparam);
                int my = GET_Y_LPARAM(lparam);

                MouseEvent me{};
                me.type = MouseEventType::Up;
                me.button = MouseButton::Left;
                me.screen_pos = {mx, my};
                me.world_pos = state->renderer->viewport().screen_to_world(mx, my);
                me.ctrl_down = (wparam & MK_CONTROL) != 0;
                me.shift_down = (wparam & MK_SHIFT) != 0;
                state->renderer->dispatch_mouse_event(me);
            }
            return 0;
        }

        case WM_RBUTTONDOWN: {
            if (state && state->renderer) {
                int mx = GET_X_LPARAM(lparam);
                int my = GET_Y_LPARAM(lparam);

                MouseEvent me{};
                me.type = MouseEventType::Down;
                me.button = MouseButton::Right;
                me.screen_pos = {mx, my};
                me.world_pos = state->renderer->viewport().screen_to_world(mx, my);
                me.ctrl_down = (wparam & MK_CONTROL) != 0;
                me.shift_down = (wparam & MK_SHIFT) != 0;
                state->renderer->dispatch_mouse_event(me);
            }
            return 0;
        }

        case WM_RBUTTONUP: {
            if (state && state->renderer) {
                int mx = GET_X_LPARAM(lparam);
                int my = GET_Y_LPARAM(lparam);

                MouseEvent me{};
                me.type = MouseEventType::Up;
                me.button = MouseButton::Right;
                me.screen_pos = {mx, my};
                me.world_pos = state->renderer->viewport().screen_to_world(mx, my);
                me.ctrl_down = (wparam & MK_CONTROL) != 0;
                me.shift_down = (wparam & MK_SHIFT) != 0;
                state->renderer->dispatch_mouse_event(me);
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            if (state && state->renderer) {
                int mx = GET_X_LPARAM(lparam);
                int my = GET_Y_LPARAM(lparam);

                MouseEvent me{};
                me.type = MouseEventType::Move;
                me.button = state->is_dragging ? MouseButton::Left : MouseButton::None;
                me.screen_pos = {mx, my};
                me.world_pos = state->renderer->viewport().screen_to_world(mx, my);
                me.ctrl_down = (wparam & MK_CONTROL) != 0;
                me.shift_down = (wparam & MK_SHIFT) != 0;

                state->renderer->dispatch_mouse_event(me);

                if (state->is_dragging) {
                    int dx = mx - state->last_mouse_x;
                    int dy = my - state->last_mouse_y;
                    state->last_mouse_x = mx;
                    state->last_mouse_y = my;

                    state->renderer->viewport().pan(dx, dy);
                }

                // If not continuously animated, immediately redraw on mouse motion
                if (!state->is_animated) {
                    state->renderer->render(state->get_elapsed_time());
                    state->renderer->present();
                }
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            if (state && state->renderer) {
                int delta = GET_WHEEL_DELTA_WPARAM(wparam);
                double factor = (delta > 0) ? 1.15 : (1.0 / 1.15);

                POINT pt{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                ScreenToClient(hwnd, &pt);

                MouseEvent me{};
                me.type = MouseEventType::Wheel;
                me.button = MouseButton::Middle;
                me.screen_pos = {pt.x, pt.y};
                me.world_pos = state->renderer->viewport().screen_to_world(pt.x, pt.y);
                me.wheel_delta = delta;
                me.ctrl_down = (wparam & MK_CONTROL) != 0;
                me.shift_down = (wparam & MK_SHIFT) != 0;
                state->renderer->dispatch_mouse_event(me);

                state->renderer->viewport().zoom(factor, Point2D(static_cast<double>(pt.x), static_cast<double>(pt.y)));
                if (!state->is_animated) {
                    state->renderer->render(state->get_elapsed_time());
                    state->renderer->present();
                }
            }
            return 0;
        }

        case WM_LBUTTONDBLCLK: {
            if (state && state->renderer) {
                state->renderer->viewport().set_bounds(Rect2D(-10.0, 10.0, -10.0, 10.0));
                if (!state->is_animated) {
                    state->renderer->render(state->get_elapsed_time());
                    state->renderer->present();
                }
            }
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

} // anonymous namespace

Win32WindowHandle create_win32_window(
    const Win32WindowDesc& desc,
    IWindowRenderer* renderer
) {
    HINSTANCE hinstance = GetModuleHandle(nullptr);
    const char* class_name = "FormulaicRenderWindowClass";

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = ManagedWndProc;
    wc.hInstance = hinstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = class_name;

    RegisterClassExA(&wc);

    auto* state = new WindowState();
    state->renderer = renderer;

    HWND hwnd = CreateWindowExA(
        0,
        class_name,
        desc.title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        desc.width, desc.height,
        nullptr,
        nullptr,
        hinstance,
        state
    );

    if (hwnd && desc.show) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }

    return Win32WindowHandle{reinterpret_cast<void*>(hwnd), reinterpret_cast<void*>(hinstance)};
}

void destroy_win32_window(Win32WindowHandle handle) {
    if (handle.hwnd) {
        HWND hwnd = reinterpret_cast<HWND>(handle.hwnd);
        WindowState* state = reinterpret_cast<WindowState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        DestroyWindow(hwnd);
        delete state;
    }
}

void run_win32_message_loop(IWindowRenderer* renderer, bool run_animation) {
    MSG msg{};

    HWND hwnd = renderer ? reinterpret_cast<HWND>(renderer->native_handle()) : nullptr;
    WindowState* state = hwnd ? reinterpret_cast<WindowState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)) : nullptr;
    if (state) {
        state->is_animated = run_animation;
        state->start_time = std::chrono::high_resolution_clock::now();
    }

    using clock = std::chrono::high_resolution_clock;
    auto last_frame = clock::now();

    while (msg.message != WM_QUIT) {
        // Drain all pending messages so mouse and window inputs are immediately consumed
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (msg.message == WM_QUIT) break;

        if (run_animation && renderer && renderer->is_attached()) {
            double elapsed = state ? state->get_elapsed_time() : 0.0;
            renderer->render(elapsed);
            renderer->present();

            // Smooth 60 FPS frame pacing (~16.6 ms per frame)
            auto now = clock::now();
            auto delta_us = std::chrono::duration_cast<std::chrono::microseconds>(now - last_frame).count();
            if (delta_us < 16000) {
                DWORD sleep_ms = static_cast<DWORD>((16000 - delta_us) / 1000);
                if (sleep_ms > 0) {
                    Sleep(sleep_ms);
                }
            }
            last_frame = clock::now();
        } else {
            WaitMessage();
        }
    }
}

} // namespace formulaic

#endif // _WIN32
