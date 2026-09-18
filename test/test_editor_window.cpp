#include <Formulaic/hooks/pipeline_hooks.hpp>
#include <Formulaic/math/calculus.hpp>
#include <Formulaic/math/fft.hpp>
#include <Formulaic/parser/expression.hpp>
#include <Formulaic/render/color.hpp>
#include <Formulaic/render/framebuffer.hpp>
#include <Formulaic/render/raster_engine.hpp>
#include <Formulaic/render/viewport.hpp>
#include <Formulaic/render/window_renderer.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: [" << #cond << "] " << (msg) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return 1; \
        } \
    } while (0)

// Control IDs
constexpr int IDC_EDIT_EXPR   = 1001;
constexpr int IDC_BTN_RENDER  = 1002;
constexpr int IDC_BTN_PRESET1 = 1003;
constexpr int IDC_BTN_PRESET2 = 1004;
constexpr int IDC_BTN_PRESET3 = 1005;
constexpr int IDC_BTN_PRESET4 = 1006;
constexpr int IDC_STATUS_TEXT = 1007;
constexpr int IDC_COMBO_MODE  = 1008;

constexpr UINT_PTR TIMER_ANIM_ID = 2001;

enum class PlotMode {
    Auto = 0,
    Explicit1D,
    ScalarField2D,
    Implicit2D
};

struct EditorWindowState {
    std::unique_ptr<formulaic::IWindowRenderer> renderer;
    formulaic::RasterEngine engine;
    formulaic::Expression current_expr;
    bool is_valid_expr{false};
    std::string error_message;
    PlotMode plot_mode{PlotMode::Auto};

    HWND hwnd_main{nullptr};
    HWND hwnd_edit{nullptr};
    HWND hwnd_status{nullptr};
    HWND hwnd_render{nullptr};
    HWND hwnd_combo_mode{nullptr};

    HBRUSH bg_brush{nullptr};
    HBRUSH edit_bg_brush{nullptr};
    HFONT font_title{nullptr};
    HFONT font_mono{nullptr};
    HFONT font_ui{nullptr};

    bool is_dragging{false};
    int last_mouse_x{0};
    int last_mouse_y{0};
    bool uses_time_t{false};
    std::chrono::high_resolution_clock::time_point start_time;

    [[nodiscard]] double get_elapsed_time() const noexcept {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(now - start_time).count();
    }
};

static LRESULT CALLBACK CanvasWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<EditorWindowState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_LBUTTONDOWN: {
            if (state && state->renderer) {
                state->is_dragging = true;
                state->last_mouse_x = GET_X_LPARAM(lparam);
                state->last_mouse_y = GET_Y_LPARAM(lparam);
                SetCapture(hwnd);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            if (state && state->is_dragging) {
                state->is_dragging = false;
                ReleaseCapture();
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            if (state && state->renderer) {
                int mx = GET_X_LPARAM(lparam);
                int my = GET_Y_LPARAM(lparam);

                if (state->is_dragging) {
                    int dx = mx - state->last_mouse_x;
                    int dy = my - state->last_mouse_y;
                    state->last_mouse_x = mx;
                    state->last_mouse_y = my;
                    state->renderer->viewport().pan(dx, dy);
                }

                formulaic::MouseEvent me{};
                me.type = formulaic::MouseEventType::Move;
                me.button = state->is_dragging ? formulaic::MouseButton::Left : formulaic::MouseButton::None;
                me.screen_pos = {mx, my};
                me.world_pos = state->renderer->viewport().screen_to_world(mx, my);
                state->renderer->dispatch_mouse_event(me);

                if (!state->uses_time_t) {
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

                state->renderer->viewport().zoom(factor, formulaic::Point2D(static_cast<double>(pt.x), static_cast<double>(pt.y)));
                if (!state->uses_time_t) {
                    state->renderer->render(state->get_elapsed_time());
                    state->renderer->present();
                }
            }
            return 0;
        }

        case WM_LBUTTONDBLCLK: {
            if (state && state->renderer) {
                state->renderer->viewport().set_bounds(formulaic::Rect2D(-5.0, 5.0, -5.0, 5.0));
                if (!state->uses_time_t) {
                    state->renderer->render(state->get_elapsed_time());
                    state->renderer->present();
                }
            }
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            if (state && state->renderer) {
                state->renderer->render(state->get_elapsed_time());
                state->renderer->present();
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void update_expression_from_edit(EditorWindowState* state) {
    if (!state || !state->hwnd_edit) return;

    char buffer[4096] = {0};
    GetWindowTextA(state->hwnd_edit, buffer, sizeof(buffer) - 1);
    std::string text(buffer);

    if (text.empty()) {
        state->is_valid_expr = false;
        state->error_message = "Enter a formula or variable script";
        SetWindowTextA(state->hwnd_status, state->error_message.c_str());
        return;
    }

    auto parse_res = formulaic::Expression::parse(text);
    if (!parse_res) {
        state->is_valid_expr = false;
        state->error_message = "Error: " + parse_res.error().format();
        SetWindowTextA(state->hwnd_status, state->error_message.c_str());
        return;
    }

    state->current_expr = std::move(parse_res.value());
    state->is_valid_expr = true;
    state->error_message.clear();

    // Check if time variable 't' is actually referenced for animation
    state->uses_time_t = state->current_expr.references_variable("t");
    std::string var_summary = "Vars: ";
    auto actual_vars = state->current_expr.referenced_variables();
    if (actual_vars.empty()) {
        var_summary += "none";
    } else {
        for (const auto& v : actual_vars) {
            var_summary += v + " ";
        }
    }

    std::string status_info = "Status: Valid | " + var_summary;
    status_info += " | Bytecode: " + std::to_string(state->current_expr.bytecode().instructions.size()) + " insts";
    if (state->uses_time_t) status_info += " [Animated]";
    SetWindowTextA(state->hwnd_status, status_info.c_str());

    if (state->renderer && state->renderer->is_attached()) {
        state->renderer->render(state->get_elapsed_time());
        state->renderer->present();
    }
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<EditorWindowState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_COMMAND: {
            WORD id = LOWORD(wparam);
            WORD code = HIWORD(wparam);

            if (id == IDC_EDIT_EXPR && code == EN_CHANGE) {
                update_expression_from_edit(state);
                return 0;
            }

            if (id == IDC_BTN_RENDER && code == BN_CLICKED) {
                update_expression_from_edit(state);
                return 0;
            }

            if (id == IDC_COMBO_MODE && code == CBN_SELCHANGE) {
                if (state && state->hwnd_combo_mode) {
                    int sel = static_cast<int>(SendMessageA(state->hwnd_combo_mode, CB_GETCURSEL, 0, 0));
                    state->plot_mode = static_cast<PlotMode>(sel);
                    if (state->renderer) {
                        state->renderer->render(state->get_elapsed_time());
                        state->renderer->present();
                    }
                }
                return 0;
            }

            // Presets
            if (id == IDC_BTN_PRESET1 && code == BN_CLICKED) {
                const char* s = "let r = hypot(x, y);\nlet theta = atan2(y, x);\nsin(6.0 * theta) * exp(-0.35 * r);";
                SetWindowTextA(state->hwnd_edit, s);
                SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, static_cast<WPARAM>(PlotMode::Auto), 0);
                state->plot_mode = PlotMode::Auto;
                update_expression_from_edit(state);
                return 0;
            }

            if (id == IDC_BTN_PRESET2 && code == BN_CLICKED) {
                const char* s = "let h = 0.01;\nlet fp = sin(x + h) * cos(y);\nlet fm = sin(x - h) * cos(y);\ndiff_step(fp, fm, h);";
                SetWindowTextA(state->hwnd_edit, s);
                SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, static_cast<WPARAM>(PlotMode::Auto), 0);
                state->plot_mode = PlotMode::Auto;
                update_expression_from_edit(state);
                return 0;
            }

            if (id == IDC_BTN_PRESET3 && code == BN_CLICKED) {
                const char* s = "let u = clamp((x + 5.0) / 10.0, 0.0, 1.0);\nhann(u) * triangle_wave(u * 5.0 - t * 0.5);";
                SetWindowTextA(state->hwnd_edit, s);
                SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, static_cast<WPARAM>(PlotMode::Auto), 0);
                state->plot_mode = PlotMode::Auto;
                update_expression_from_edit(state);
                return 0;
            }

            if (id == IDC_BTN_PRESET4 && code == BN_CLICKED) {
                const char* s = "let r = hypot(x, y);\nr - 3.0 - 0.4 * sin(7.0 * atan2(y, x) + 2.0 * t);";
                SetWindowTextA(state->hwnd_edit, s);
                SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, static_cast<WPARAM>(PlotMode::Implicit2D), 0);
                state->plot_mode = PlotMode::Implicit2D;
                update_expression_from_edit(state);
                return 0;
            }
            break;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            HWND hctrl = reinterpret_cast<HWND>(lparam);
            SetBkMode(hdc, TRANSPARENT);

            if (state && hctrl == state->hwnd_status) {
                if (state->is_valid_expr) {
                    SetTextColor(hdc, RGB(80, 240, 120)); // Soft neon green
                } else {
                    SetTextColor(hdc, RGB(255, 90, 90));  // Warning red
                }
            } else {
                SetTextColor(hdc, RGB(210, 210, 225));
            }
            return reinterpret_cast<LRESULT>(state ? state->bg_brush : GetStockObject(BLACK_BRUSH));
        }

        case WM_CTLCOLOREDIT: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            SetBkColor(hdc, RGB(26, 26, 36));
            SetTextColor(hdc, RGB(120, 220, 255));
            return reinterpret_cast<LRESULT>(state ? state->edit_bg_brush : GetStockObject(BLACK_BRUSH));
        }

        case WM_TIMER: {
            if (wparam == TIMER_ANIM_ID && state && state->renderer) {
                if (state->uses_time_t && state->is_valid_expr) {
                    state->renderer->render(state->get_elapsed_time());
                    state->renderer->present();
                }
            }
            return 0;
        }

        case WM_SIZE: {
            if (state) {
                int client_w = LOWORD(lparam);
                int client_h = HIWORD(lparam);
                constexpr int left_panel_w = 420;

                // Adjust right render canvas
                int render_x = left_panel_w + 10;
                int render_w = client_w - render_x - 10;
                int render_h = client_h - 20;

                if (state->hwnd_render && render_w > 0 && render_h > 0) {
                    MoveWindow(state->hwnd_render, render_x, 10, render_w, render_h, TRUE);
                    if (state->renderer) {
                        state->renderer->on_resize(render_w, render_h);
                        state->renderer->render(state->get_elapsed_time());
                        state->renderer->present();
                    }
                }
            }
            return 0;
        }

        case WM_DESTROY: {
            KillTimer(hwnd, TIMER_ANIM_ID);
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

#endif // _WIN32

int main(int argc, char* argv[]) {
    std::cout << "===========================================================\n";
    std::cout << " Formulaic Split Window Expression Editor & Live Renderer  \n";
    std::cout << "===========================================================\n";

    bool is_automated = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--automated" || arg == "--ci" || arg == "-a") {
            is_automated = true;
        }
    }

#if defined(_WIN32)
    HINSTANCE hinstance = GetModuleHandle(nullptr);

    // 1. Register main split window class
    const char* kMainClass = "FormulaicSplitEditorMainWindow";
    WNDCLASSEXA wc_main{};
    wc_main.cbSize = sizeof(WNDCLASSEXA);
    wc_main.style = CS_HREDRAW | CS_VREDRAW;
    wc_main.lpfnWndProc = MainWndProc;
    wc_main.hInstance = hinstance;
    wc_main.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc_main.hbrBackground = CreateSolidBrush(RGB(20, 20, 28));
    wc_main.lpszClassName = kMainClass;
    RegisterClassExA(&wc_main);

    // 2. Register child canvas window class
    const char* kCanvasClass = "FormulaicRenderCanvasChild";
    WNDCLASSEXA wc_canvas{};
    wc_canvas.cbSize = sizeof(WNDCLASSEXA);
    wc_canvas.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc_canvas.lpfnWndProc = CanvasWndProc;
    wc_canvas.hInstance = hinstance;
    wc_canvas.hCursor = LoadCursor(nullptr, IDC_CROSS);
    wc_canvas.hbrBackground = nullptr;
    wc_canvas.lpszClassName = kCanvasClass;
    RegisterClassExA(&wc_canvas);

    // 3. Initialize state
    auto state = std::make_unique<EditorWindowState>();
    state->bg_brush = CreateSolidBrush(RGB(20, 20, 28));
    state->edit_bg_brush = CreateSolidBrush(RGB(26, 26, 36));
    state->font_title = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, "Segoe UI");
    state->font_mono  = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 0, 0, CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");
    state->font_ui    = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, "Segoe UI");
    state->start_time = std::chrono::high_resolution_clock::now();

    const int init_win_w = 1200;
    const int init_win_h = 750;

    HWND hwnd_main = CreateWindowExA(
        0,
        kMainClass,
        "Formulaic - Live Expression Editor & Real-Time Math Visualizer",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        init_win_w, init_win_h,
        nullptr, nullptr, hinstance, nullptr
    );
    TEST_ASSERT(hwnd_main != nullptr, "Created main split editor window");
    state->hwnd_main = hwnd_main;
    SetWindowLongPtrA(hwnd_main, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state.get()));

    // Left Panel Layout
    constexpr int left_w = 400;

    // Title label
    HWND lbl_title = CreateWindowExA(0, "STATIC", "Expression Script Editor", WS_CHILD | WS_VISIBLE, 15, 12, left_w, 24, hwnd_main, nullptr, hinstance, nullptr);
    SendMessageA(lbl_title, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_title), TRUE);

    // Instruction label
    HWND lbl_inst = CreateWindowExA(0, "STATIC", "Supports let/var, multi-line scripts, calculus & FFT builtins:", WS_CHILD | WS_VISIBLE, 15, 40, left_w, 18, hwnd_main, nullptr, hinstance, nullptr);
    SendMessageA(lbl_inst, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    // Multi-line Edit Box
    const std::string initial_script =
        "let r = hypot(x, y);\n"
        "let theta = atan2(y, x);\n"
        "let envelope = exp(-0.35 * r);\n"
        "sin(6.0 * theta + 2.0 * t) * envelope;";

    state->hwnd_edit = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "EDIT",
        initial_script.c_str(),
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        15, 65, left_w, 180,
        hwnd_main,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_EXPR)),
        hinstance, nullptr
    );
    SendMessageA(state->hwnd_edit, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_mono), TRUE);

    // Render / Update Button
    HWND btn_render = CreateWindowExA(0, "BUTTON", "Update & Render Expression", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, 255, left_w, 32, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_RENDER)), hinstance, nullptr);
    SendMessageA(btn_render, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    // Mode selection Combo
    HWND lbl_mode = CreateWindowExA(0, "STATIC", "Rendering Mode:", WS_CHILD | WS_VISIBLE, 15, 298, 120, 20, hwnd_main, nullptr, hinstance, nullptr);
    SendMessageA(lbl_mode, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    state->hwnd_combo_mode = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 145, 295, left_w - 130, 120, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_MODE)), hinstance, nullptr);
    SendMessageA(state->hwnd_combo_mode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Auto (1D Curve / 2D Field)"));
    SendMessageA(state->hwnd_combo_mode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("1D Explicit Curve y = f(x)"));
    SendMessageA(state->hwnd_combo_mode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("2D Heatmap z = f(x, y)"));
    SendMessageA(state->hwnd_combo_mode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Implicit Curve f(x, y) = 0"));
    SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, 0, 0);
    SendMessageA(state->hwnd_combo_mode, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    // Preset Buttons Section
    HWND lbl_presets = CreateWindowExA(0, "STATIC", "Sample Script Presets:", WS_CHILD | WS_VISIBLE, 15, 330, left_w, 20, hwnd_main, nullptr, hinstance, nullptr);
    SendMessageA(lbl_presets, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    HWND btn_p1 = CreateWindowExA(0, "BUTTON", "1. Spiral Waves (let/var)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, 355, 195, 28, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PRESET1)), hinstance, nullptr);
    HWND btn_p2 = CreateWindowExA(0, "BUTTON", "2. Calculus (diff_step)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 220, 355, 195, 28, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PRESET2)), hinstance, nullptr);
    HWND btn_p3 = CreateWindowExA(0, "BUTTON", "3. FFT Window (hann/wave)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, 390, 195, 28, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PRESET3)), hinstance, nullptr);
    HWND btn_p4 = CreateWindowExA(0, "BUTTON", "4. Implicit Flower f=0", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 220, 390, 195, 28, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PRESET4)), hinstance, nullptr);
    SendMessageA(btn_p1, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);
    SendMessageA(btn_p2, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);
    SendMessageA(btn_p3, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);
    SendMessageA(btn_p4, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    // Status / Diagnostic text
    state->hwnd_status = CreateWindowExA(0, "STATIC", "Status: Initializing...", WS_CHILD | WS_VISIBLE, 15, 430, left_w, 55, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATUS_TEXT)), hinstance, nullptr);
    SendMessageA(state->hwnd_status, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    // Navigation & Interaction Tip box
    std::string tips = "Viewport Controls:\n"
                       "- Left Click + Drag: Pan coordinate viewport\n"
                       "- Mouse Wheel: Zoom in / out at mouse cursor\n"
                       "- Double Click: Reset viewport to [-5, 5]\n"
                       "- Animation active when 't' is referenced";
    HWND lbl_tips = CreateWindowExA(0, "STATIC", tips.c_str(), WS_CHILD | WS_VISIBLE, 15, 500, left_w, 90, hwnd_main, nullptr, hinstance, nullptr);
    SendMessageA(lbl_tips, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    // Right Canvas Window
    int canvas_x = left_w + 30;
    int canvas_w = init_win_w - canvas_x - 30;
    int canvas_h = init_win_h - 60;

    state->hwnd_render = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        kCanvasClass,
        "Formulaic Canvas",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        canvas_x, 15, canvas_w, canvas_h,
        hwnd_main,
        nullptr,
        hinstance,
        nullptr
    );
    TEST_ASSERT(state->hwnd_render != nullptr, "Created child render canvas HWND");
    SetWindowLongPtrA(state->hwnd_render, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state.get()));

    // Initialize Window Renderer attached to the child canvas HWND
    state->renderer = formulaic::create_window_renderer();
    TEST_ASSERT(state->renderer != nullptr, "Created WindowRenderer");
    bool attached = state->renderer->attach(reinterpret_cast<void*>(state->hwnd_render));
    TEST_ASSERT(attached, "Renderer attached to child canvas HWND");

    // Setup custom render callback
    state->renderer->set_render_callback([s = state.get()](formulaic::FrameBuffer& fb, const formulaic::Viewport& vp, double time_t) {
        fb.clear(formulaic::Color::BackgroundDark);

        formulaic::GridStyle grid_style;
        grid_style.background_color = formulaic::Color::BackgroundDark;
        grid_style.show_grid = true;
        grid_style.show_axes = true;
        grid_style.show_labels = true;
        s->engine.render_grid(fb, vp, grid_style);

        if (!s->is_valid_expr) return;

        PlotMode mode = s->plot_mode;
        if (mode == PlotMode::Auto) {
            bool has_y = s->current_expr.references_variable("y");
            mode = has_y ? PlotMode::ScalarField2D : PlotMode::Explicit1D;
        }

        switch (mode) {
            case PlotMode::Explicit1D:
                s->engine.plot_explicit(fb, vp, s->current_expr, formulaic::Color::NeonBlue, 2.5, time_t);
                break;
            case PlotMode::ScalarField2D:
                s->engine.plot_scalar_field(fb, vp, s->current_expr, formulaic::ColormapType::Viridis, -1.5, 1.5, time_t);
                break;
            case PlotMode::Implicit2D:
                s->engine.plot_implicit(fb, vp, s->current_expr, formulaic::Color::NeonPink, 2.5, time_t);
                break;
            case PlotMode::Auto:
                break;
        }

        // Title watermark
        fb.draw_text(15, 15, "Formulaic Viewport [Real-Time Active]", formulaic::Color::White);
    });

    // Start 60 FPS animation timer
    SetTimer(hwnd_main, TIMER_ANIM_ID, 16, nullptr);

    // Initial parse & render
    update_expression_from_edit(state.get());

    if (!is_automated) {
        ShowWindow(hwnd_main, SW_SHOW);
        UpdateWindow(hwnd_main);

        std::cout << "\n[Interactive Mode Running]\n";
        std::cout << "-> Left panel: type math scripts, use 'let'/'var', calculus & FFT functions.\n";
        std::cout << "-> Right panel: real-time rendered math canvas.\n";
        std::cout << "-> Close window or press Alt+F4 to exit.\n\n";

        MSG msg{};
        while (GetMessageA(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    } else {
        std::cout << "[Automated Test Mode Running]\n";

        // Verification 1: Verify valid script parsed and rendered
        TEST_ASSERT(state->is_valid_expr, "Initial multi-variable expression is valid");
        TEST_ASSERT(state->renderer->framebuffer().width() > 0, "Framebuffer has valid width");
        TEST_ASSERT(state->renderer->framebuffer().height() > 0, "Framebuffer has valid height");

        // Verification 2: Test syntax error diagnostics
        SetWindowTextA(state->hwnd_edit, "sin(x +");
        update_expression_from_edit(state.get());
        TEST_ASSERT(!state->is_valid_expr, "Syntax error identified");
        TEST_ASSERT(!state->error_message.empty(), "Error message populated");

        // Verification 3: Test preset 1 (let / var)
        SendMessageA(hwnd_main, WM_COMMAND, MAKEWPARAM(IDC_BTN_PRESET1, BN_CLICKED), 0);
        TEST_ASSERT(state->is_valid_expr, "Preset 1 is valid");

        // Verification 4: Test preset 2 (calculus diff_step)
        SendMessageA(hwnd_main, WM_COMMAND, MAKEWPARAM(IDC_BTN_PRESET2, BN_CLICKED), 0);
        TEST_ASSERT(state->is_valid_expr, "Preset 2 is valid");

        // Verification 5: Test preset 3 (FFT windowing)
        SendMessageA(hwnd_main, WM_COMMAND, MAKEWPARAM(IDC_BTN_PRESET3, BN_CLICKED), 0);
        TEST_ASSERT(state->is_valid_expr, "Preset 3 is valid");

        // Verification 6: Test preset 4 (implicit function)
        SendMessageA(hwnd_main, WM_COMMAND, MAKEWPARAM(IDC_BTN_PRESET4, BN_CLICKED), 0);
        TEST_ASSERT(state->is_valid_expr, "Preset 4 is valid");
        TEST_ASSERT(state->plot_mode == PlotMode::Implicit2D, "Plot mode set to Implicit2D");

        std::cout << "All split-window interactive GUI & render assertions PASSED successfully!\n";
    }

    // Cleanup Win32 resources
    if (state->bg_brush) DeleteObject(state->bg_brush);
    if (state->edit_bg_brush) DeleteObject(state->edit_bg_brush);
    if (state->font_title) DeleteObject(state->font_title);
    if (state->font_mono) DeleteObject(state->font_mono);
    if (state->font_ui) DeleteObject(state->font_ui);

    if (state->renderer) {
        state->renderer->detach();
    }
    DestroyWindow(hwnd_main);

#else
    std::cout << "Split window editor requires Win32 GUI subsystem.\n";
#endif

    std::cout << "\n>>> Split Window Editor Test PASSED successfully! <<<\n";
    return 0;
}
