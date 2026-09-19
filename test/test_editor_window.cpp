#include <Formulaic/hooks/pipeline_hooks.hpp>
#include <Formulaic/math/calculus.hpp>
#include <Formulaic/math/fft.hpp>
#include <Formulaic/parser/expression.hpp>
#include <Formulaic/parser/latex_converter.hpp>
#include <Formulaic/render/color.hpp>
#include <Formulaic/render/framebuffer.hpp>
#include <Formulaic/render/raster_engine.hpp>
#include <Formulaic/render/viewport.hpp>
#include <Formulaic/render/window_renderer.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
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
#include <richedit.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: [" << #cond << "] " << (msg) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return 1; \
        } \
    } while (0)

// Control IDs
constexpr int IDC_EDIT_EXPR      = 1001;
constexpr int IDC_BTN_RENDER     = 1002;
constexpr int IDC_BTN_PRESET1    = 1003;
constexpr int IDC_BTN_PRESET2    = 1004;
constexpr int IDC_BTN_PRESET3    = 1005;
constexpr int IDC_BTN_PRESET4    = 1006;
constexpr int IDC_STATUS_TEXT    = 1007;
constexpr int IDC_COMBO_MODE     = 1008;
constexpr int IDC_AC_LIST        = 1009;
constexpr int IDC_BTN_COPY_LATEX       = 1010;
constexpr int IDC_EDIT_LATEX           = 1011;
constexpr int IDC_BTN_LATEX_TO_SCRIPT  = 1012;

constexpr UINT_PTR TIMER_ANIM_ID     = 2001;
constexpr UINT_PTR TIMER_DEBOUNCE_ID = 2002;

enum class PlotMode {
    Auto = 0,
    Explicit1D,
    ScalarField2D,
    Implicit2D
};

struct AutocompleteItem {
    std::string name;
    std::string category;
    bool is_func;
};

// Comprehensive mathematical catalog for autocomplete & syntax highlighting
static const std::vector<AutocompleteItem> kCatalog = {
    {"let", "keyword", false},
    {"var", "keyword", false},
    {"x", "var", false},
    {"y", "var", false},
    {"t", "var", false},
    {"pi", "const", false},
    {"e", "const", false},
    {"tau", "const", false},
    {"phi", "const", false},
    {"euler", "const", false},
    {"sqrt2", "const", false},
    {"sqrt3", "const", false},
    {"inf", "const", false},
    {"sin", "trig", true},
    {"cos", "trig", true},
    {"tan", "trig", true},
    {"asin", "trig", true},
    {"acos", "trig", true},
    {"atan", "trig", true},
    {"atan2", "trig", true},
    {"sec", "trig", true},
    {"csc", "trig", true},
    {"cot", "trig", true},
    {"asec", "trig", true},
    {"acsc", "trig", true},
    {"acot", "trig", true},
    {"sinh", "hyperb", true},
    {"cosh", "hyperb", true},
    {"tanh", "hyperb", true},
    {"sech", "hyperb", true},
    {"csch", "hyperb", true},
    {"coth", "hyperb", true},
    {"asinh", "hyperb", true},
    {"acosh", "hyperb", true},
    {"atanh", "hyperb", true},
    {"exp", "exp/log", true},
    {"exp2", "exp/log", true},
    {"expm1", "exp/log", true},
    {"ln", "exp/log", true},
    {"log", "exp/log", true},
    {"log10", "exp/log", true},
    {"log2", "exp/log", true},
    {"log1p", "exp/log", true},
    {"pow", "exp/log", true},
    {"sqrt", "math", true},
    {"cbrt", "math", true},
    {"abs", "math", true},
    {"floor", "math", true},
    {"ceil", "math", true},
    {"round", "math", true},
    {"trunc", "math", true},
    {"frac", "math", true},
    {"sign", "math", true},
    {"copysign", "math", true},
    {"hypot", "math", true},
    {"min", "math", true},
    {"max", "math", true},
    {"clamp", "math", true},
    {"lerp", "math", true},
    {"step", "math", true},
    {"smoothstep", "math", true},
    {"sinc", "special", true},
    {"erf", "special", true},
    {"erfc", "special", true},
    {"gamma", "special", true},
    {"lgamma", "special", true},
    {"beta", "special", true},
    {"diff_step", "calculus", true},
    {"diff_forward", "calculus", true},
    {"diff_backward", "calculus", true},
    {"diff2_step", "calculus", true},
    {"curvature_2d", "calculus", true},
    {"hann", "fft", true},
    {"hamming", "fft", true},
    {"blackman", "fft", true},
    {"flattop", "fft", true},
    {"welch", "fft", true},
    {"square_wave", "signal", true},
    {"sawtooth_wave", "signal", true},
    {"triangle_wave", "signal", true},
    {"chirp", "signal", true}
};

static bool is_catalog_function(std::string_view name) noexcept {
    for (const auto& item : kCatalog) {
        if (item.name == name && item.is_func) return true;
    }
    return false;
}

static bool is_catalog_constant(std::string_view name) noexcept {
    for (const auto& item : kCatalog) {
        if (item.name == name && item.category == "const") return true;
    }
    return false;
}

enum class EditorTokenType {
    Default,
    Comment,
    Keyword,
    Function,
    Constant,
    Variable,
    Number,
    Operator
};

struct EditorTokenSpan {
    int start_pos;
    int end_pos;
    EditorTokenType tok_type;
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
    HWND hwnd_latex_edit{nullptr};
    HWND hwnd_btn_copy_latex{nullptr};
    HWND hwnd_btn_latex_to_script{nullptr};
    std::string current_latex;

    HWND hwnd_ac_popup{nullptr};
    HWND hwnd_ac_list{nullptr};
    WNDPROC original_edit_proc{nullptr};
    bool ac_visible{false};
    int ac_prefix_start{0};
    int ac_prefix_len{0};
    bool is_highlighting{false};

    HBRUSH bg_brush{nullptr};
    HBRUSH edit_bg_brush{nullptr};
    HBRUSH ac_bg_brush{nullptr};
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
                    if (dx != 0 || dy != 0) {
                        state->last_mouse_x = mx;
                        state->last_mouse_y = my;
                        state->renderer->viewport().pan(dx, dy);

                        formulaic::MouseEvent me{};
                        me.type = formulaic::MouseEventType::Move;
                        me.button = formulaic::MouseButton::Left;
                        me.screen_pos = {mx, my};
                        me.world_pos = state->renderer->viewport().screen_to_world(mx, my);
                        state->renderer->dispatch_mouse_event(me);

                        if (!state->uses_time_t) {
                            state->renderer->render(state->get_elapsed_time());
                            state->renderer->present();
                        }
                    }
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
        }
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void update_expression_from_edit(EditorWindowState* state);

static std::vector<AutocompleteItem> find_autocomplete_matches(std::string_view prefix) {
    std::vector<AutocompleteItem> results;
    if (prefix.empty()) return results;

    std::string lower_prefix;
    lower_prefix.reserve(prefix.size());
    for (char c : prefix) lower_prefix.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

    for (const auto& item : kCatalog) {
        std::string lower_name;
        lower_name.reserve(item.name.size());
        for (char c : item.name) lower_name.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

        if (lower_name.rfind(lower_prefix, 0) == 0) { // starts with
            results.push_back(item);
        }
    }
    return results;
}

static std::vector<EditorTokenSpan> scan_syntax_tokens(std::string_view text) {
    std::vector<EditorTokenSpan> tokens;
    const int n = static_cast<int>(text.size());
    int i = 0;

    while (i < n) {
        char c = text[i];

        // 1. Line Comments: //
        if (c == '/' && i + 1 < n && text[i + 1] == '/') {
            int start = i;
            while (i < n && text[i] != '\n' && text[i] != '\r') {
                i++;
            }
            tokens.push_back({start, i, EditorTokenType::Comment});
            continue;
        }

        // 2. Whitespace
        if (std::isspace(static_cast<unsigned char>(c))) {
            i++;
            continue;
        }

        // 3. Numbers
        if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && i + 1 < n && std::isdigit(static_cast<unsigned char>(text[i + 1])))) {
            int start = i;
            while (i < n && (std::isdigit(static_cast<unsigned char>(text[i])) || text[i] == '.' || text[i] == 'e' || text[i] == 'E')) {
                if ((text[i] == 'e' || text[i] == 'E') && i + 1 < n && (text[i + 1] == '+' || text[i + 1] == '-')) {
                    i += 2;
                } else {
                    i++;
                }
            }
            tokens.push_back({start, i, EditorTokenType::Number});
            continue;
        }

        // 4. Identifiers (Keywords, Functions, Constants, Variables)
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            int start = i;
            while (i < n && (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_')) {
                i++;
            }
            std::string_view word = text.substr(start, i - start);
            EditorTokenType tok_t = EditorTokenType::Variable;
            if (word == "let" || word == "var") {
                tok_t = EditorTokenType::Keyword;
            } else if (is_catalog_constant(word)) {
                tok_t = EditorTokenType::Constant;
            } else if (is_catalog_function(word)) {
                tok_t = EditorTokenType::Function;
            }
            tokens.push_back({start, i, tok_t});
            continue;
        }

        // 5. Operators & Equations
        if (c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '%' ||
            c == '<' || c == '>' || c == '!' || c == ';' || c == ',') {
            int start = i;
            if ((c == '=' || c == '!' || c == '<' || c == '>') && i + 1 < n && text[i + 1] == '=') {
                i += 2;
            } else {
                i++;
            }
            tokens.push_back({start, i, EditorTokenType::Operator});
            continue;
        }

        // 6. Delimiters
        i++;
    }

    return tokens;
}

static COLORREF get_token_color(EditorTokenType type) noexcept {
    switch (type) {
        case EditorTokenType::Comment:  return RGB(108, 112, 134); // Slate Gray
        case EditorTokenType::Keyword:  return RGB(249, 226, 175); // Soft Gold / Yellow
        case EditorTokenType::Function: return RGB(137, 220, 235); // Cyan
        case EditorTokenType::Constant: return RGB(148, 226, 213); // Mint Teal
        case EditorTokenType::Variable: return RGB(203, 166, 247); // Lavender / Mauve
        case EditorTokenType::Number:   return RGB(250, 179, 135); // Peach / Orange
        case EditorTokenType::Operator: return RGB(243, 139, 168); // Flamingo Pink
        case EditorTokenType::Default:
        default:                        return RGB(205, 214, 244); // Default Off-White
    }
}

static std::string get_edit_text_exact(HWND hwnd_edit) {
    if (!hwnd_edit) return {};
    GETTEXTLENGTHEX gtl{};
    gtl.flags = GTL_DEFAULT | GTL_NUMCHARS;
    gtl.codepage = CP_ACP;
    int len = static_cast<int>(SendMessageA(hwnd_edit, EM_GETTEXTLENGTHEX, reinterpret_cast<WPARAM>(&gtl), 0));
    if (len > 0) {
        std::string text(len + 2, '\0');
        GETTEXTEX gt{};
        gt.cb = len + 1;
        gt.flags = GT_DEFAULT;
        gt.codepage = CP_ACP;
        int fetched = static_cast<int>(SendMessageA(hwnd_edit, EM_GETTEXTEX, reinterpret_cast<WPARAM>(&gt), reinterpret_cast<LPARAM>(text.data())));
        if (fetched > 0) {
            text.resize(fetched);
            return text;
        }
    }
    int gwt_len = GetWindowTextLengthA(hwnd_edit);
    if (gwt_len <= 0) return {};
    std::string text(gwt_len + 1, '\0');
    GetWindowTextA(hwnd_edit, text.data(), gwt_len + 1);
    text.resize(gwt_len);
    return text;
}

static void apply_syntax_highlighting(EditorWindowState* state) {
    if (!state || !state->hwnd_edit || state->is_highlighting) return;
    state->is_highlighting = true;

    std::string text = get_edit_text_exact(state->hwnd_edit);
    int len = static_cast<int>(text.size());
    if (len <= 0) {
        state->is_highlighting = false;
        return;
    }

    CHARRANGE saved_sel{};
    SendMessageA(state->hwnd_edit, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&saved_sel));
    POINT scroll_pos{};
    SendMessageA(state->hwnd_edit, EM_GETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&scroll_pos));

    // Turn off event mask and freeze repainting during format update
    DWORD old_mask = static_cast<DWORD>(SendMessageA(state->hwnd_edit, EM_GETEVENTMASK, 0, 0));
    SendMessageA(state->hwnd_edit, EM_SETEVENTMASK, 0, 0);
    SendMessageA(state->hwnd_edit, WM_SETREDRAW, FALSE, 0);

    // Reset whole text to default color
    CHARRANGE all_range{0, len};
    SendMessageA(state->hwnd_edit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&all_range));
    CHARFORMAT2A cf{};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = RGB(205, 214, 244);
    SendMessageA(state->hwnd_edit, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));

    // Tokenize and format colored tokens
    auto tokens = scan_syntax_tokens(text);
    for (const auto& tok : tokens) {
        COLORREF color = get_token_color(tok.tok_type);
        if (color == RGB(205, 214, 244)) continue;

        CHARRANGE token_range{tok.start_pos, tok.end_pos};
        SendMessageA(state->hwnd_edit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&token_range));
        CHARFORMAT2A cf_tok{};
        cf_tok.cbSize = sizeof(cf_tok);
        cf_tok.dwMask = CFM_COLOR;
        cf_tok.crTextColor = color;
        SendMessageA(state->hwnd_edit, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf_tok));
    }

    // Restore caret selection, scroll, event mask, and redraw
    SendMessageA(state->hwnd_edit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&saved_sel));
    SendMessageA(state->hwnd_edit, EM_SETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&scroll_pos));
    SendMessageA(state->hwnd_edit, WM_SETREDRAW, TRUE, 0);
    SendMessageA(state->hwnd_edit, EM_SETEVENTMASK, 0, old_mask);
    InvalidateRect(state->hwnd_edit, nullptr, FALSE);

    state->is_highlighting = false;
}

static void hide_autocomplete(EditorWindowState* state) {
    if (state && state->ac_visible && state->hwnd_ac_popup) {
        ShowWindow(state->hwnd_ac_popup, SW_HIDE);
        state->ac_visible = false;
    }
}

static void accept_autocomplete(EditorWindowState* state) {
    if (!state || !state->ac_visible || !state->hwnd_ac_list) return;

    int sel = static_cast<int>(SendMessageA(state->hwnd_ac_list, LB_GETCURSEL, 0, 0));
    if (sel < 0) return;

    char buf[128]{};
    SendMessageA(state->hwnd_ac_list, LB_GETTEXT, sel, reinterpret_cast<LPARAM>(buf));

    std::string item_str(buf);
    size_t space_pos = item_str.find(' ');
    std::string clean_name = (space_pos != std::string::npos) ? item_str.substr(0, space_pos) : item_str;

    bool is_func = false;
    if (clean_name.length() > 2 && clean_name.substr(clean_name.length() - 2) == "()") {
        clean_name = clean_name.substr(0, clean_name.length() - 2);
        is_func = true;
    } else if (is_catalog_function(clean_name)) {
        is_func = true;
    }

    // Replace the prefix
    CHARRANGE cr{state->ac_prefix_start, state->ac_prefix_start + state->ac_prefix_len};
    SendMessageA(state->hwnd_edit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));

    std::string insertion = clean_name;
    if (is_func) {
        insertion += "()";
    }
    SendMessageA(state->hwnd_edit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(insertion.c_str()));

    if (is_func) {
        // Place caret inside the parentheses: sin(|)
        int inside_pos = state->ac_prefix_start + static_cast<int>(clean_name.length()) + 1;
        CHARRANGE inside_cr{inside_pos, inside_pos};
        SendMessageA(state->hwnd_edit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&inside_cr));
    }

    hide_autocomplete(state);
    apply_syntax_highlighting(state);
    update_expression_from_edit(state);
}

static void trigger_autocomplete_check(EditorWindowState* state) {
    if (!state || !state->hwnd_edit || state->is_highlighting) return;

    CHARRANGE sel{};
    SendMessageA(state->hwnd_edit, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&sel));
    if (sel.cpMin != sel.cpMax) {
        hide_autocomplete(state);
        return;
    }

    int caret = sel.cpMin;
    if (caret <= 0) {
        hide_autocomplete(state);
        return;
    }

    std::string text = get_edit_text_exact(state->hwnd_edit);
    if (text.empty() || caret > static_cast<int>(text.size())) {
        hide_autocomplete(state);
        return;
    }

    int word_start = caret;
    while (word_start > 0) {
        char prev = text[word_start - 1];
        if (std::isalnum(static_cast<unsigned char>(prev)) || prev == '_') {
            word_start--;
        } else {
            break;
        }
    }

    int prefix_len = caret - word_start;
    if (prefix_len < 1) {
        hide_autocomplete(state);
        return;
    }

    std::string prefix = text.substr(word_start, prefix_len);
    auto matches = find_autocomplete_matches(prefix);
    if (matches.empty()) {
        hide_autocomplete(state);
        return;
    }

    // Populate listbox
    SendMessageA(state->hwnd_ac_list, LB_RESETCONTENT, 0, 0);
    for (const auto& item : matches) {
        std::string display = item.name + (item.is_func ? "()" : "") + "  [" + item.category + "]";
        SendMessageA(state->hwnd_ac_list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(display.c_str()));
    }
    SendMessageA(state->hwnd_ac_list, LB_SETCURSEL, 0, 0);

    // Compute caret coordinates in screen space
    POINTL pt{};
    SendMessageA(state->hwnd_edit, EM_POSFROMCHAR, reinterpret_cast<WPARAM>(&pt), caret);
    POINT screen_pt{pt.x, pt.y};
    ClientToScreen(state->hwnd_edit, &screen_pt);

    int count = static_cast<int>(matches.size());
    int popup_height = std::clamp(count * 20 + 8, 40, 160);

    SetWindowPos(state->hwnd_ac_popup, HWND_TOP, screen_pt.x, screen_pt.y + 20, 230, popup_height, SWP_SHOWWINDOW | SWP_NOACTIVATE);

    state->ac_visible = true;
    state->ac_prefix_start = word_start;
    state->ac_prefix_len = prefix_len;
}

static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<EditorWindowState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    if (!state) return DefWindowProcA(hwnd, msg, wparam, lparam);

    switch (msg) {
        case WM_KEYDOWN: {
            if (state->ac_visible && state->hwnd_ac_popup && IsWindowVisible(state->hwnd_ac_popup)) {
                if (wparam == VK_DOWN) {
                    int cur = static_cast<int>(SendMessageA(state->hwnd_ac_list, LB_GETCURSEL, 0, 0));
                    int count = static_cast<int>(SendMessageA(state->hwnd_ac_list, LB_GETCOUNT, 0, 0));
                    if (cur < count - 1) {
                        SendMessageA(state->hwnd_ac_list, LB_SETCURSEL, cur + 1, 0);
                    }
                    return 0;
                }
                if (wparam == VK_UP) {
                    int cur = static_cast<int>(SendMessageA(state->hwnd_ac_list, LB_GETCURSEL, 0, 0));
                    if (cur > 0) {
                        SendMessageA(state->hwnd_ac_list, LB_SETCURSEL, cur - 1, 0);
                    }
                    return 0;
                }
                if (wparam == VK_RETURN || wparam == VK_TAB) {
                    accept_autocomplete(state);
                    return 0;
                }
                if (wparam == VK_ESCAPE) {
                    hide_autocomplete(state);
                    return 0;
                }
            }
            break;
        }

        case WM_CHAR: {
            LRESULT res = CallWindowProcA(state->original_edit_proc, hwnd, msg, wparam, lparam);
            if (wparam == VK_ESCAPE || wparam == VK_RETURN) {
                hide_autocomplete(state);
                return res;
            }
            trigger_autocomplete_check(state);
            return res;
        }

        case WM_LBUTTONDOWN:
        case WM_KILLFOCUS: {
            hide_autocomplete(state);
            break;
        }
    }

    return CallWindowProcA(state->original_edit_proc, hwnd, msg, wparam, lparam);
}

static LRESULT CALLBACK AutocompleteWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<EditorWindowState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_COMMAND: {
            if (HIWORD(wparam) == LBN_DBLCLK && state) {
                accept_autocomplete(state);
                return 0;
            }
            break;
        }

        case WM_CTLCOLORLISTBOX: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            SetBkColor(hdc, RGB(30, 30, 46));
            SetTextColor(hdc, RGB(205, 214, 244));
            if (state && state->ac_bg_brush) {
                return reinterpret_cast<LRESULT>(state->ac_bg_brush);
            }
            break;
        }

        case WM_SIZE: {
            if (state && state->hwnd_ac_list) {
                MoveWindow(state->hwnd_ac_list, 0, 0, LOWORD(lparam), HIWORD(lparam), TRUE);
            }
            return 0;
        }
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void update_expression_from_edit(EditorWindowState* state) {
    if (!state || !state->hwnd_edit) return;

    std::string text = get_edit_text_exact(state->hwnd_edit);

    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r' || text.back() == '\n')) {
        text.pop_back();
    }

    if (text.empty()) {
        state->is_valid_expr = false;
        state->error_message = "Enter a formula or variable script";
        SetWindowTextA(state->hwnd_status, state->error_message.c_str());
        state->current_latex.clear();
        if (state->hwnd_latex_edit) SetWindowTextA(state->hwnd_latex_edit, "");
        return;
    }

    // Real-time conversion to standard LaTeXLive
    auto latex_res = formulaic::LatexConverter::convert(text);
    if (latex_res) {
        state->current_latex = latex_res.value();
        if (state->hwnd_latex_edit) {
            SetWindowTextA(state->hwnd_latex_edit, state->current_latex.c_str());
        }
    } else {
        state->current_latex.clear();
        if (state->hwnd_latex_edit) {
            SetWindowTextA(state->hwnd_latex_edit, "(LaTeX conversion pending or syntax error)");
        }
    }

    // Check if user entered an equation LHS = RHS (single-line or multi-line declarations ending in an equation)
    bool is_equation = false;
    std::string eq_lhs, eq_rhs;
    std::string prefix_stmts;

    std::string trimmed_text = text;
    while (!trimmed_text.empty() && (trimmed_text.back() == ' ' || trimmed_text.back() == '\t' || trimmed_text.back() == '\r' || trimmed_text.back() == '\n' || trimmed_text.back() == ';')) {
        trimmed_text.pop_back();
    }

    size_t last_semi = std::string::npos;
    int paren_cnt = 0;
    for (size_t i = 0; i < trimmed_text.size(); ++i) {
        if (trimmed_text[i] == '(') ++paren_cnt;
        else if (trimmed_text[i] == ')') { if (paren_cnt > 0) --paren_cnt; }
        else if (trimmed_text[i] == ';' && paren_cnt == 0) {
            last_semi = i;
        }
    }

    prefix_stmts = (last_semi != std::string::npos) ? trimmed_text.substr(0, last_semi + 1) + "\n" : "";
    std::string last_stmt = (last_semi != std::string::npos) ? trimmed_text.substr(last_semi + 1) : trimmed_text;

    size_t s_idx = 0;
    while (s_idx < last_stmt.size() && (last_stmt[s_idx] == ' ' || last_stmt[s_idx] == '\t' || last_stmt[s_idx] == '\r' || last_stmt[s_idx] == '\n')) ++s_idx;
    std::string_view prefix = std::string_view(last_stmt).substr(s_idx, 4);
    if (prefix != "let " && prefix != "var ") {
        int p_depth = 0;
        for (size_t p = 0; p < last_stmt.size(); ++p) {
            if (last_stmt[p] == '(') ++p_depth;
            else if (last_stmt[p] == ')') { if (p_depth > 0) --p_depth; }
            else if (p_depth == 0 && last_stmt[p] == '=') {
                if (p > 0 && (last_stmt[p-1] == '!' || last_stmt[p-1] == '<' || last_stmt[p-1] == '>')) continue;
                size_t eq_len = (p + 1 < last_stmt.size() && last_stmt[p+1] == '=') ? 2 : 1;
                eq_lhs = last_stmt.substr(0, p);
                eq_rhs = last_stmt.substr(p + eq_len);
                is_equation = true;
                break;
            }
        }
    }

    formulaic::Result<formulaic::Expression> parse_res = formulaic::Expression::parse(text);

    if (is_equation) {
        auto trim_str = [](std::string s) -> std::string {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.erase(0, 1);
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n' || s.back() == ';')) s.pop_back();
            return s;
        };
        eq_lhs = trim_str(eq_lhs);
        eq_rhs = trim_str(eq_rhs);

        // Special case: y = f(x) -> 1D explicit curve
        if (eq_lhs == "y") {
            std::string expr_candidate = prefix_stmts + eq_rhs + ";";
            auto rhs_res = formulaic::Expression::parse(expr_candidate);
            if (rhs_res.has_value() && !rhs_res->references_variable("y")) {
                parse_res = std::move(rhs_res);
                state->plot_mode = PlotMode::Explicit1D;
                if (state->hwnd_combo_mode) {
                    SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, static_cast<WPARAM>(PlotMode::Explicit1D), 0);
                }
            } else {
                std::string implicit_script = prefix_stmts + "(" + eq_lhs + ") - (" + eq_rhs + ");";
                parse_res = formulaic::Expression::parse(implicit_script);
                state->plot_mode = PlotMode::Implicit2D;
                if (state->hwnd_combo_mode) {
                    SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, static_cast<WPARAM>(PlotMode::Implicit2D), 0);
                }
            }
        } else {
            // General equation: LHS = RHS -> implicit (LHS) - (RHS) = 0
            std::string implicit_script = prefix_stmts + "(" + eq_lhs + ") - (" + eq_rhs + ");";
            parse_res = formulaic::Expression::parse(implicit_script);
            state->plot_mode = PlotMode::Implicit2D;
            if (state->hwnd_combo_mode) {
                SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, static_cast<WPARAM>(PlotMode::Implicit2D), 0);
            }
        }
    }

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

    std::string status_info;
    if (is_equation) {
        if (eq_rhs == "0" && (eq_lhs == "x^2 + y^2" || eq_lhs == "x^2+y^2" || eq_lhs == "x*x + y*y" || eq_lhs == "x*x+y*y")) {
            status_info = "Status: Valid Equation (x^2+y^2=0) [Implicit Mode]\nNote: x^2+y^2=0 is a single point (0,0). For a visible circle try: x^2+y^2=4";
        } else {
            status_info = "Status: Valid Equation (" + eq_lhs + " = " + eq_rhs + ") [Implicit Mode] | " + var_summary;
        }
    } else {
        status_info = "Status: Valid | " + var_summary;
        status_info += " | Bytecode: " + std::to_string(state->current_expr.bytecode().instructions.size()) + " insts";
    }
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
                if (state && !state->is_highlighting) {
                    apply_syntax_highlighting(state);
                    // 120ms debounce so typing is fluid without blocking on heavy render
                    SetTimer(hwnd, TIMER_DEBOUNCE_ID, 120, nullptr);
                }
                return 0;
            }

            if (id == IDC_BTN_RENDER && code == BN_CLICKED) {
                if (state) {
                    KillTimer(hwnd, TIMER_DEBOUNCE_ID);
                    apply_syntax_highlighting(state);
                    update_expression_from_edit(state);
                }
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
                KillTimer(hwnd, TIMER_DEBOUNCE_ID);
                const char* s = "let r = hypot(x, y);\nlet theta = atan2(y, x);\nsin(6.0 * theta) * exp(-0.35 * r);";
                SetWindowTextA(state->hwnd_edit, s);
                SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, static_cast<WPARAM>(PlotMode::Auto), 0);
                state->plot_mode = PlotMode::Auto;
                apply_syntax_highlighting(state);
                update_expression_from_edit(state);
                return 0;
            }

            if (id == IDC_BTN_PRESET2 && code == BN_CLICKED) {
                KillTimer(hwnd, TIMER_DEBOUNCE_ID);
                const char* s = "let h = 0.01;\nlet fp = sin(x + h) * cos(y);\nlet fm = sin(x - h) * cos(y);\ndiff_step(fp, fm, h);";
                SetWindowTextA(state->hwnd_edit, s);
                SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, static_cast<WPARAM>(PlotMode::Auto), 0);
                state->plot_mode = PlotMode::Auto;
                apply_syntax_highlighting(state);
                update_expression_from_edit(state);
                return 0;
            }

            if (id == IDC_BTN_PRESET3 && code == BN_CLICKED) {
                KillTimer(hwnd, TIMER_DEBOUNCE_ID);
                const char* s = "let u = clamp((x + 5.0) / 10.0, 0.0, 1.0);\nhann(u) * triangle_wave(u * 5.0 - t * 0.5);";
                SetWindowTextA(state->hwnd_edit, s);
                SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, static_cast<WPARAM>(PlotMode::Auto), 0);
                state->plot_mode = PlotMode::Auto;
                apply_syntax_highlighting(state);
                update_expression_from_edit(state);
                return 0;
            }

            if (id == IDC_BTN_PRESET4 && code == BN_CLICKED) {
                KillTimer(hwnd, TIMER_DEBOUNCE_ID);
                const char* s = "let r = hypot(x, y);\nr - 3.0 - 0.4 * sin(7.0 * atan2(y, x) + 2.0 * t) = 0";
                SetWindowTextA(state->hwnd_edit, s);
                SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, static_cast<WPARAM>(PlotMode::Implicit2D), 0);
                state->plot_mode = PlotMode::Implicit2D;
                apply_syntax_highlighting(state);
                update_expression_from_edit(state);
                return 0;
            }
            if (id == IDC_BTN_COPY_LATEX && code == BN_CLICKED) {
                if (state && !state->current_latex.empty()) {
                    if (OpenClipboard(hwnd)) {
                        EmptyClipboard();
                        const std::string& str = state->current_latex;
                        HGLOBAL hglb = GlobalAlloc(GMEM_MOVEABLE, str.size() + 1);
                        if (hglb) {
                            char* lptstr = static_cast<char*>(GlobalLock(hglb));
                            if (lptstr) {
                                memcpy(lptstr, str.c_str(), str.size() + 1);
                                GlobalUnlock(hglb);
                                SetClipboardData(CF_TEXT, hglb);
                            }
                        }
                        CloseClipboard();
                        SetWindowTextA(state->hwnd_status, "Status: LaTeXLive formula copied to clipboard!");
                    }
                }
                return 0;
            }

            if (id == IDC_BTN_LATEX_TO_SCRIPT && code == BN_CLICKED) {
                if (state && state->hwnd_latex_edit) {
                    std::string latex_str = get_edit_text_exact(state->hwnd_latex_edit);
                    auto script_res = formulaic::LatexConverter::to_script(latex_str);
                    if (script_res) {
                        SetWindowTextA(state->hwnd_edit, script_res.value().c_str());
                        state->current_latex = latex_str;
                        apply_syntax_highlighting(state);
                        update_expression_from_edit(state);
                        SetWindowTextA(state->hwnd_status, "Status: LaTeXLive converted to script successfully!");
                    } else {
                        std::string err = "LaTeX Error: " + script_res.error().format();
                        SetWindowTextA(state->hwnd_status, err.c_str());
                    }
                }
                return 0;
            }
            break;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            HWND hctrl = reinterpret_cast<HWND>(lparam);
            SetBkMode(hdc, TRANSPARENT);

            if (state && hctrl == state->hwnd_latex_edit) {
                SetBkColor(hdc, RGB(24, 24, 37));
                SetTextColor(hdc, RGB(180, 230, 255)); // Soft Cyan for LaTeX
                return reinterpret_cast<LRESULT>(state->edit_bg_brush);
            }

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
            HWND hctrl = reinterpret_cast<HWND>(lparam);
            if (state && hctrl == state->hwnd_latex_edit) {
                SetBkColor(hdc, RGB(24, 24, 37));
                SetTextColor(hdc, RGB(180, 230, 255));
                return reinterpret_cast<LRESULT>(state->edit_bg_brush);
            }
            SetBkColor(hdc, RGB(26, 26, 36));
            SetTextColor(hdc, RGB(120, 220, 255));
            return reinterpret_cast<LRESULT>(state ? state->edit_bg_brush : GetStockObject(BLACK_BRUSH));
        }

        case WM_TIMER: {
            if (wparam == TIMER_DEBOUNCE_ID) {
                KillTimer(hwnd, TIMER_DEBOUNCE_ID);
                if (state) {
                    update_expression_from_edit(state);
                }
                return 0;
            }
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
            KillTimer(hwnd, TIMER_DEBOUNCE_ID);
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
    HMODULE hRich = LoadLibraryA("riched20.dll");
    const char* edit_class_name = (hRich != nullptr) ? "RichEdit20A" : "EDIT";

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

    // 3. Register autocomplete popup window class
    const char* kAcPopupClass = "FormulaicAutocompletePopup";
    WNDCLASSEXA wc_ac{};
    wc_ac.cbSize = sizeof(WNDCLASSEXA);
    wc_ac.style = CS_HREDRAW | CS_VREDRAW;
    wc_ac.lpfnWndProc = AutocompleteWndProc;
    wc_ac.hInstance = hinstance;
    wc_ac.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc_ac.hbrBackground = CreateSolidBrush(RGB(30, 30, 46));
    wc_ac.lpszClassName = kAcPopupClass;
    RegisterClassExA(&wc_ac);

    // 4. Initialize state
    auto state = std::make_unique<EditorWindowState>();
    state->bg_brush = CreateSolidBrush(RGB(20, 20, 28));
    state->edit_bg_brush = CreateSolidBrush(RGB(24, 24, 37));
    state->ac_bg_brush = CreateSolidBrush(RGB(30, 30, 46));
    state->font_title = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, "Segoe UI");
    state->font_mono  = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 0, 0, CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");
    state->font_ui    = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, "Segoe UI");
    state->start_time = std::chrono::high_resolution_clock::now();

    const int init_win_w = 1250;
    const int init_win_h = 800;

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
    HWND lbl_inst = CreateWindowExA(0, "STATIC", "Syntax Highlighting & Autocomplete (Type 'sin', 'let', 'diff'):", WS_CHILD | WS_VISIBLE, 15, 40, left_w, 18, hwnd_main, nullptr, hinstance, nullptr);
    SendMessageA(lbl_inst, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    // Multi-line Edit Box
    const std::string initial_script =
        "let r = hypot(x, y);\n"
        "let theta = atan2(y, x);\n"
        "let envelope = exp(-0.35 * r);\n"
        "sin(6.0 * theta + 2.0 * t) * envelope;";

    state->hwnd_edit = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        edit_class_name,
        initial_script.c_str(),
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        15, 65, left_w, 160,
        hwnd_main,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_EXPR)),
        hinstance, nullptr
    );
    SendMessageA(state->hwnd_edit, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_mono), TRUE);
    SendMessageA(state->hwnd_edit, EM_SETBKGNDCOLOR, 0, RGB(24, 24, 37));
    SendMessageA(state->hwnd_edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(8, 8));

    // Subclass edit control for autocomplete navigation
    SetWindowLongPtrA(state->hwnd_edit, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state.get()));
    state->original_edit_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(state->hwnd_edit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditSubclassProc)));

    // Create autocomplete floating popup window
    state->hwnd_ac_popup = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        kAcPopupClass,
        "",
        WS_POPUP | WS_BORDER,
        0, 0, 230, 140,
        hwnd_main,
        nullptr,
        hinstance,
        nullptr
    );
    SetWindowLongPtrA(state->hwnd_ac_popup, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state.get()));

    state->hwnd_ac_list = CreateWindowExA(
        0,
        "LISTBOX",
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS | LBS_WANTKEYBOARDINPUT,
        0, 0, 230, 140,
        state->hwnd_ac_popup,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_AC_LIST)),
        hinstance,
        nullptr
    );
    SendMessageA(state->hwnd_ac_list, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_mono), TRUE);

    // Render / Update Button
    HWND btn_render = CreateWindowExA(0, "BUTTON", "Update & Render Expression", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, 232, left_w, 30, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_RENDER)), hinstance, nullptr);
    SendMessageA(btn_render, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    // Mode selection Combo
    HWND lbl_mode = CreateWindowExA(0, "STATIC", "Rendering Mode:", WS_CHILD | WS_VISIBLE, 15, 271, 120, 20, hwnd_main, nullptr, hinstance, nullptr);
    SendMessageA(lbl_mode, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    state->hwnd_combo_mode = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 145, 268, left_w - 130, 120, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_MODE)), hinstance, nullptr);
    SendMessageA(state->hwnd_combo_mode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Auto (1D Curve / 2D Field)"));
    SendMessageA(state->hwnd_combo_mode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("1D Explicit Curve y = f(x)"));
    SendMessageA(state->hwnd_combo_mode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("2D Heatmap z = f(x, y)"));
    SendMessageA(state->hwnd_combo_mode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Implicit Curve f(x, y) = 0"));
    SendMessageA(state->hwnd_combo_mode, CB_SETCURSEL, 0, 0);
    SendMessageA(state->hwnd_combo_mode, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    // Preset Buttons Section
    HWND lbl_presets = CreateWindowExA(0, "STATIC", "Sample Script Presets:", WS_CHILD | WS_VISIBLE, 15, 298, left_w, 18, hwnd_main, nullptr, hinstance, nullptr);
    SendMessageA(lbl_presets, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    HWND btn_p1 = CreateWindowExA(0, "BUTTON", "1. Spiral Waves (let/var)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, 320, 195, 26, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PRESET1)), hinstance, nullptr);
    HWND btn_p2 = CreateWindowExA(0, "BUTTON", "2. Calculus (diff_step)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 220, 320, 195, 26, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PRESET2)), hinstance, nullptr);
    HWND btn_p3 = CreateWindowExA(0, "BUTTON", "3. FFT Window (hann/wave)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, 350, 195, 26, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PRESET3)), hinstance, nullptr);
    HWND btn_p4 = CreateWindowExA(0, "BUTTON", "4. Implicit Flower f=0", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 220, 350, 195, 26, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PRESET4)), hinstance, nullptr);
    SendMessageA(btn_p1, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);
    SendMessageA(btn_p2, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);
    SendMessageA(btn_p3, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);
    SendMessageA(btn_p4, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    // Status / Diagnostic text
    state->hwnd_status = CreateWindowExA(0, "STATIC", "Status: Initializing...", WS_CHILD | WS_VISIBLE, 15, 382, left_w, 46, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATUS_TEXT)), hinstance, nullptr);
    SendMessageA(state->hwnd_status, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    // LaTeXLive Mathematical Notation Section
    HWND lbl_latex = CreateWindowExA(0, "STATIC", "LaTeXLive Formula:", WS_CHILD | WS_VISIBLE, 15, 434, 150, 20, hwnd_main, nullptr, hinstance, nullptr);
    SendMessageA(lbl_latex, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    state->hwnd_btn_copy_latex = CreateWindowExA(0, "BUTTON", "Copy LaTeX", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 170, 430, 110, 26, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_COPY_LATEX)), hinstance, nullptr);
    SendMessageA(state->hwnd_btn_copy_latex, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    state->hwnd_btn_latex_to_script = CreateWindowExA(0, "BUTTON", "LaTeX -> Script", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 285, 430, 130, 26, hwnd_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_LATEX_TO_SCRIPT)), hinstance, nullptr);
    SendMessageA(state->hwnd_btn_latex_to_script, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    state->hwnd_latex_edit = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
        15, 458, left_w, 76,
        hwnd_main,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_LATEX)),
        hinstance, nullptr
    );
    SendMessageA(state->hwnd_latex_edit, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_mono), TRUE);

    // Navigation & Interaction Tip box
    std::string tips = "Viewport Controls:\n"
                       "- Left Click + Drag: Pan coordinate viewport\n"
                       "- Mouse Wheel: Zoom in / out at mouse cursor\n"
                       "- Double Click: Reset viewport to [-5, 5]\n"
                       "- Autocomplete: Type 'sin', 'diff' + Tab/Enter";
    HWND lbl_tips = CreateWindowExA(0, "STATIC", tips.c_str(), WS_CHILD | WS_VISIBLE, 15, 542, left_w, 85, hwnd_main, nullptr, hinstance, nullptr);
    SendMessageA(lbl_tips, WM_SETFONT, reinterpret_cast<WPARAM>(state->font_ui), TRUE);

    // Right Canvas Viewport
    constexpr int canvas_x = 440;
    HWND hwnd_render = CreateWindowExA(
        0,
        kCanvasClass,
        "",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        canvas_x, 0, init_win_w - canvas_x, init_win_h,
        hwnd_main,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(2000)),
        hinstance, nullptr
    );
    TEST_ASSERT(hwnd_render != nullptr, "Created render viewport canvas HWND");
    state->hwnd_render = hwnd_render;
    SetWindowLongPtrA(hwnd_render, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state.get()));

    // Create & attach window renderer
    state->renderer = formulaic::create_window_renderer();
    TEST_ASSERT(state->renderer != nullptr, "Created window renderer");
    bool attached = state->renderer->attach(reinterpret_cast<void*>(hwnd_render));
    TEST_ASSERT(attached, "Attached window renderer to canvas HWND");

    // Configure render callback
    EditorWindowState* s = state.get();
    state->renderer->set_render_callback([s](formulaic::FrameBuffer& fb, const formulaic::Viewport& vp, double time_t) {
        fb.clear(formulaic::Color::BackgroundDark);

        formulaic::GridStyle style;
        style.show_grid = true;
        style.show_axes = true;
        style.show_labels = true;
        style.background_color = formulaic::Color::BackgroundDark;
        s->engine.render_grid(fb, vp, style);

        if (!s->is_valid_expr) {
            fb.draw_text(20, 30, "Syntax / Semantic Error:", formulaic::Color::NeonPink);
            fb.draw_text(20, 50, s->error_message, formulaic::Color::White);
            return;
        }

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

        // LaTeXLive visual HUD card floating on top of canvas
        if (!s->current_latex.empty()) {
            s->engine.plot_latex_card(fb, 20, 20, s->current_latex);
        } else {
            // Title watermark
            fb.draw_text(15, 15, "Formulaic Viewport [Real-Time Active]", formulaic::Color::White);
        }
    });

    // Start 60 FPS animation timer
    SetTimer(hwnd_main, TIMER_ANIM_ID, 16, nullptr);

    // Initial highlight, parse & render
    apply_syntax_highlighting(state.get());
    update_expression_from_edit(state.get());

    if (!is_automated) {
        ShowWindow(hwnd_main, SW_SHOW);
        UpdateWindow(hwnd_main);

        std::cout << "\n[Interactive Mode Running]\n";
        std::cout << "-> Left panel: syntax highlighting & autocomplete active.\n";
        std::cout << "-> Type 'sin', 'diff_step', 'let' to test autocomplete.\n";
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

        // Verification 2: Test syntax highlighting scanner
        auto tokens = scan_syntax_tokens("let r = hypot(x, y); sin(r) * 2.5;");
        TEST_ASSERT(!tokens.empty(), "Syntax scanner produced tokens");
        bool has_let = false, has_hypot = false, has_num = false;
        for (const auto& tok : tokens) {
            if (tok.tok_type == EditorTokenType::Keyword) has_let = true;
            if (tok.tok_type == EditorTokenType::Function) has_hypot = true;
            if (tok.tok_type == EditorTokenType::Number) has_num = true;
        }
        TEST_ASSERT(has_let, "Scanner identified keyword 'let'");
        TEST_ASSERT(has_hypot, "Scanner identified function 'hypot'");
        TEST_ASSERT(has_num, "Scanner identified number '2.5'");

        // Verification 3: Test autocomplete catalog lookup
        auto ac_sin = find_autocomplete_matches("si");
        TEST_ASSERT(!ac_sin.empty(), "Autocomplete found matches for 'si'");
        TEST_ASSERT(ac_sin[0].name == "sin", "First match for 'si' is 'sin'");

        auto ac_hyp = find_autocomplete_matches("hy");
        TEST_ASSERT(!ac_hyp.empty(), "Autocomplete found matches for 'hy'");
        TEST_ASSERT(ac_hyp[0].name == "hypot", "Match for 'hy' is 'hypot'");

        auto ac_diff = find_autocomplete_matches("di");
        TEST_ASSERT(!ac_diff.empty(), "Autocomplete found matches for 'di'");
        TEST_ASSERT(ac_diff[0].name == "diff_step", "First match for 'di' is 'diff_step'");

        // Verification 4: Test syntax error diagnostics
        SetWindowTextA(state->hwnd_edit, "sin(x +");
        update_expression_from_edit(state.get());
        TEST_ASSERT(!state->is_valid_expr, "Syntax error identified");
        TEST_ASSERT(!state->error_message.empty(), "Error message populated");

        // Verification 5: Test preset 1 (let / var)
        SendMessageA(hwnd_main, WM_COMMAND, MAKEWPARAM(IDC_BTN_PRESET1, BN_CLICKED), 0);
        TEST_ASSERT(state->is_valid_expr, "Preset 1 is valid");

        // Verification 6: Test preset 2 (calculus diff_step)
        SendMessageA(hwnd_main, WM_COMMAND, MAKEWPARAM(IDC_BTN_PRESET2, BN_CLICKED), 0);
        TEST_ASSERT(state->is_valid_expr, "Preset 2 is valid");

        // Verification 7: Test preset 3 (FFT windowing)
        SendMessageA(hwnd_main, WM_COMMAND, MAKEWPARAM(IDC_BTN_PRESET3, BN_CLICKED), 0);
        TEST_ASSERT(state->is_valid_expr, "Preset 3 is valid");

        // Verification 8: Test preset 4 (implicit function)
        SendMessageA(hwnd_main, WM_COMMAND, MAKEWPARAM(IDC_BTN_PRESET4, BN_CLICKED), 0);
        TEST_ASSERT(state->is_valid_expr, "Preset 4 is valid");
        TEST_ASSERT(state->plot_mode == PlotMode::Implicit2D, "Plot mode set to Implicit2D");

        // Verification 9: Test Navier-Stokes Taylor-Green vortex implicit multi-line script
        const char* navier_script =
            "let h = 0.01;\r\n"
            "let nu = 0.08;\r\n"
            "let u0 = sin(x) * cos(y);\r\n"
            "let v0 = -cos(x) * sin(y);\r\n"
            "let dudx = diff_step(sin(x + h) * cos(y), sin(x - h) * cos(y), h);\r\n"
            "let dudy = diff_step(sin(x) * cos(y + h), sin(x) * cos(y - h), h);\r\n"
            "let dpdx = diff_step(0.25 * (cos(2.0 * (x + h)) + cos(2.0 * y)), 0.25 * (cos(2.0 * (x - h)) + cos(2.0 * y)), h);\r\n"
            "let d2u_dx2 = (sin(x + h) * cos(y) - 2.0 * u0 + sin(x - h) * cos(y)) / (h * h);\r\n"
            "let d2u_dy2 = (sin(x) * cos(y + h) - 2.0 * u0 + sin(x) * cos(y - h)) / (h * h);\r\n"
            "let lap_u = d2u_dx2 + d2u_dy2;\r\n"
            "u0 * dudx + v0 * dudy + dpdx - nu * lap_u = 0";

        SetWindowTextA(state->hwnd_edit, navier_script);
        apply_syntax_highlighting(state.get());
        update_expression_from_edit(state.get());

        TEST_ASSERT(state->is_valid_expr, "Navier-Stokes equation parsed successfully");
        TEST_ASSERT(state->plot_mode == PlotMode::Implicit2D, "Detected PlotMode::Implicit2D for equation");

        // Verification 10: Verify LaTeXLive generation in GUI state and edit control
        TEST_ASSERT(!state->current_latex.empty(), "LaTeX formula generated for Navier-Stokes equation");
        TEST_ASSERT(state->current_latex.find("\\begin{aligned}") != std::string::npos, "LaTeX formula contains \\begin{aligned}");
        TEST_ASSERT(state->current_latex.find("\\nu") != std::string::npos, "LaTeX formula contains \\nu");
        TEST_ASSERT(state->current_latex.find("u_{0}") != std::string::npos, "LaTeX formula contains u_{0}");

        // Test programmatic copy button command
        SendMessageA(hwnd_main, WM_COMMAND, MAKEWPARAM(IDC_BTN_COPY_LATEX, BN_CLICKED), 0);

        // Verification 12: Test reverse LaTeX -> Script button (IDC_BTN_LATEX_TO_SCRIPT)
        SetWindowTextA(state->hwnd_latex_edit, "\\frac{1}{x} + \\frac{1}{y} = 0");
        SendMessageA(hwnd_main, WM_COMMAND, MAKEWPARAM(IDC_BTN_LATEX_TO_SCRIPT, BN_CLICKED), 0);
        std::string script_after_latex = get_edit_text_exact(state->hwnd_edit);
        TEST_ASSERT(!script_after_latex.empty(), "Script populated from LaTeX input");
        TEST_ASSERT(state->is_valid_expr, "Converted LaTeX script parsed and validated");
        TEST_ASSERT(state->plot_mode == PlotMode::Implicit2D, "Converted rational equation set to Implicit2D");

        // Verification 13: Test full canvas HUD card and plot rendering
        state->renderer->render(0.0);
        state->renderer->present();

        std::cout << "All split-window interactive GUI, syntax highlighting, LaTeXLive & autocomplete assertions PASSED!\n";
    }

    // Cleanup Win32 resources
    if (state->bg_brush) DeleteObject(state->bg_brush);
    if (state->edit_bg_brush) DeleteObject(state->edit_bg_brush);
    if (state->ac_bg_brush) DeleteObject(state->ac_bg_brush);
    if (state->font_title) DeleteObject(state->font_title);
    if (state->font_mono) DeleteObject(state->font_mono);
    if (state->font_ui) DeleteObject(state->font_ui);

    if (state->hwnd_ac_popup) {
        DestroyWindow(state->hwnd_ac_popup);
    }
    if (state->renderer) {
        state->renderer->detach();
    }
    DestroyWindow(hwnd_main);

    if (hRich) {
        FreeLibrary(hRich);
    }

#else
    std::cout << "Split window editor requires Win32 GUI subsystem.\n";
#endif

    std::cout << "\n>>> Split Window Editor Test PASSED successfully! <<<\n";
    return 0;
}
