#include <Formulaic/render/latex_render_module.hpp>
#include <Formulaic/render/raster_engine.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#endif

namespace formulaic {

Result<Expression> LatexRenderModule::compile_latex(
    std::string_view latex_text,
    const std::vector<std::string>& variable_names
) {
    return Expression::parse_latex(latex_text, variable_names);
}

void LatexRenderModule::render_latex_plot(
    FrameBuffer& fb,
    const Viewport& vp,
    std::string_view latex_text,
    Color color,
    double line_thickness,
    double time_t
) {
    auto expr_res = compile_latex(latex_text);
    if (!expr_res) {
        fb.draw_text(20, 20, "LaTeX Parse Error: " + expr_res.error().format(), Color::NeonPink);
        return;
    }

    const auto& expr = expr_res.value();
    RasterEngine engine;

    std::string text_str(latex_text);
    bool is_implicit = (text_str.find('=') != std::string::npos && text_str.find("y = ") != 0);

    if (is_implicit) {
        engine.plot_implicit(fb, vp, expr, color, line_thickness, time_t);
    } else {
        bool has_y = expr.references_variable("y");
        if (has_y) {
            engine.plot_scalar_field(fb, vp, expr, ColormapType::Viridis, -1.5, 1.5, time_t);
        } else {
            engine.plot_explicit(fb, vp, expr, color, line_thickness, time_t);
        }
    }
}

namespace {

#ifdef _WIN32

// Process-lifetime RAII manager for GDI+
struct GdiplusManager {
    ULONG_PTR token{0};
    bool ok{false};

    GdiplusManager() {
        Gdiplus::GdiplusStartupInput input;
        if (Gdiplus::GdiplusStartup(&token, &input, nullptr) == Gdiplus::Ok) {
            ok = true;
        }
    }

    ~GdiplusManager() {
        if (ok) {
            Gdiplus::GdiplusShutdown(token);
        }
    }

    static bool init() {
        static GdiplusManager mgr;
        return mgr.ok;
    }
};

std::wstring utf8_to_wide(std::string_view utf8_str) {
    if (utf8_str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), nullptr, 0);
    if (size_needed <= 0) return std::wstring();
    std::wstring result(static_cast<size_t>(size_needed), 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), &result[0], size_needed);
    return result;
}

int get_image_encoder_clsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    auto* pImageCodecInfo = reinterpret_cast<Gdiplus::ImageCodecInfo*>(std::malloc(size));
    if (!pImageCodecInfo) return -1;

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (std::wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            std::free(pImageCodecInfo);
            return static_cast<int>(j);
        }
    }
    std::free(pImageCodecInfo);
    return -1;
}

static std::wstring resolve_font_family(const std::wstring& name) {
    if (!name.empty()) {
        Gdiplus::FontFamily fam(name.c_str());
        if (fam.IsAvailable()) return name;
    }
    Gdiplus::FontFamily cm(L"Cambria Math");
    if (cm.IsAvailable()) return L"Cambria Math";
    Gdiplus::FontFamily tnr(L"Times New Roman");
    if (tnr.IsAvailable()) return L"Times New Roman";
    return L"Arial";
}

// -------------------------------------------------------------
// Mathematical Typography Layout System for LaTeXLive Quality
// -------------------------------------------------------------

struct LayoutBox {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
    float ascent{0.0f};
    float descent{0.0f};

    virtual ~LayoutBox() = default;
    virtual void render(Gdiplus::Graphics& g, float draw_x, float draw_y, const Gdiplus::Brush& brush) = 0;
};

struct TextRunBox : public LayoutBox {
    std::wstring text;
    Gdiplus::FontStyle style{Gdiplus::FontStyleRegular};
    float font_size{22.0f};
    std::wstring family_name{L"Cambria Math"};
    float pad_left{0.0f};
    float pad_right{0.0f};

    void render(Gdiplus::Graphics& g, float draw_x, float draw_y, const Gdiplus::Brush& brush) override {
        std::wstring actual_family = resolve_font_family(family_name);
        Gdiplus::FontFamily family(actual_family.c_str());
        Gdiplus::Font font(&family, font_size, style, Gdiplus::UnitPixel);
        Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
        format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsMeasureTrailingSpaces);

        // draw_y is the baseline for this line
        float text_y = draw_y + y - ascent;
        float text_x = draw_x + x + pad_left;
        g.DrawString(text.c_str(), static_cast<int>(text.size()), &font, Gdiplus::PointF(text_x, text_y), &format, &brush);
    }
};

struct SupSubBox : public LayoutBox {
    std::unique_ptr<LayoutBox> base;
    std::unique_ptr<LayoutBox> sup;
    std::unique_ptr<LayoutBox> sub;
    float sup_shift{0.0f};
    float sub_shift{0.0f};

    void render(Gdiplus::Graphics& g, float draw_x, float draw_y, const Gdiplus::Brush& brush) override {
        if (base) {
            base->render(g, draw_x + x, draw_y + y, brush);
        }
        float script_x = draw_x + x + (base ? base->width : 0.0f);
        if (sup) {
            sup->render(g, script_x, draw_y + y - sup_shift, brush);
        }
        if (sub) {
            sub->render(g, script_x, draw_y + y + sub_shift, brush);
        }
    }
};

struct FracBox : public LayoutBox {
    std::unique_ptr<LayoutBox> num;
    std::unique_ptr<LayoutBox> den;
    float bar_y{0.0f};
    float bar_thickness{1.5f};

    void render(Gdiplus::Graphics& g, float draw_x, float draw_y, const Gdiplus::Brush& brush) override {
        float line_y = draw_y + y + bar_y;
        if (num) {
            float nx = (width - num->width) * 0.5f;
            float num_baseline = line_y - 3.0f - num->descent;
            num->render(g, draw_x + x + nx, num_baseline, brush);
        }
        if (den) {
            float dx = (width - den->width) * 0.5f;
            float den_baseline = line_y + 3.0f + den->ascent;
            den->render(g, draw_x + x + dx, den_baseline, brush);
        }

        // Draw horizontal fraction bar
        const auto* sbrush = dynamic_cast<const Gdiplus::SolidBrush*>(&brush);
        Gdiplus::Color col;
        if (sbrush) sbrush->GetColor(&col);
        else col = Gdiplus::Color(255, 255, 255, 255);

        Gdiplus::Pen pen(col, bar_thickness);
        pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
        g.DrawLine(&pen, draw_x + x + 1.0f, line_y, draw_x + x + width - 1.0f, line_y);
    }
};

struct SqrtBox : public LayoutBox {
    std::unique_ptr<LayoutBox> rad;
    float radical_width{14.0f};

    void render(Gdiplus::Graphics& g, float draw_x, float draw_y, const Gdiplus::Brush& brush) override {
        if (rad) {
            rad->render(g, draw_x + x + radical_width + 2.0f, draw_y + y, brush);
        }

        const auto* sbrush = dynamic_cast<const Gdiplus::SolidBrush*>(&brush);
        Gdiplus::Color col;
        if (sbrush) sbrush->GetColor(&col);
        else col = Gdiplus::Color(255, 255, 255, 255);

        Gdiplus::Pen pen(col, 1.5f);
        pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
        pen.SetLineJoin(Gdiplus::LineJoinMiter);

        float rx = draw_x + x;
        float ry_top = draw_y + y - ascent + 2.0f;
        float ry_bot = draw_y + y + descent;
        float ry_mid = (ry_top + ry_bot) * 0.5f;

        // Draw radical tick
        Gdiplus::PointF pts[4] = {
            Gdiplus::PointF(rx + 1.0f, ry_mid + 2.0f),
            Gdiplus::PointF(rx + 4.0f, ry_bot - 1.0f),
            Gdiplus::PointF(rx + radical_width - 1.0f, ry_top),
            Gdiplus::PointF(rx + width, ry_top)
        };
        g.DrawLines(&pen, pts, 4);
    }
};

struct DelimitedBox : public LayoutBox {
    wchar_t open_delim{L'('};
    wchar_t close_delim{L')'};
    std::unique_ptr<LayoutBox> inner;
    float delim_width{8.0f};

    void render(Gdiplus::Graphics& g, float draw_x, float draw_y, const Gdiplus::Brush& brush) override {
        const auto* sbrush = dynamic_cast<const Gdiplus::SolidBrush*>(&brush);
        Gdiplus::Color col;
        if (sbrush) sbrush->GetColor(&col);
        else col = Gdiplus::Color(255, 255, 255, 255);

        float pen_w = std::max(1.5f, std::min(2.5f, height * 0.035f));
        Gdiplus::Pen pen(col, pen_w);
        pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);

        float y_top = draw_y + y - ascent;
        float y_bot = draw_y + y + descent;
        float y_mid = (y_top + y_bot) * 0.5f;

        float open_w = (open_delim != L'.' && open_delim != 0) ? delim_width : 0.0f;
        float close_w = (close_delim != L'.' && close_delim != 0) ? delim_width : 0.0f;

        // Draw open delimiter
        float lx = draw_x + x;
        if (open_delim == L'(') {
            Gdiplus::GraphicsPath path;
            path.AddBezier(
                Gdiplus::PointF(lx + open_w - 1.0f, y_top),
                Gdiplus::PointF(lx + 1.0f, y_top + (y_mid - y_top) * 0.45f),
                Gdiplus::PointF(lx + 1.0f, y_mid + (y_bot - y_mid) * 0.55f),
                Gdiplus::PointF(lx + open_w - 1.0f, y_bot)
            );
            g.DrawPath(&pen, &path);
        } else if (open_delim == L'[') {
            g.DrawLine(&pen, lx + open_w - 1.0f, y_top, lx + 1.0f, y_top);
            g.DrawLine(&pen, lx + 1.0f, y_top, lx + 1.0f, y_bot);
            g.DrawLine(&pen, lx + 1.0f, y_bot, lx + open_w - 1.0f, y_bot);
        } else if (open_delim == L'|') {
            g.DrawLine(&pen, lx + open_w * 0.5f, y_top, lx + open_w * 0.5f, y_bot);
        } else if (open_delim == L'{') {
            Gdiplus::GraphicsPath path;
            path.AddBezier(
                Gdiplus::PointF(lx + open_w - 2.0f, y_top),
                Gdiplus::PointF(lx + open_w * 0.5f, y_top),
                Gdiplus::PointF(lx + open_w * 0.4f, y_top + 6.0f),
                Gdiplus::PointF(lx + open_w * 0.4f, y_mid - 6.0f)
            );
            path.AddBezier(
                Gdiplus::PointF(lx + open_w * 0.4f, y_mid - 6.0f),
                Gdiplus::PointF(lx + open_w * 0.4f, y_mid),
                Gdiplus::PointF(lx + 1.0f, y_mid - 1.5f),
                Gdiplus::PointF(lx + 1.0f, y_mid)
            );
            path.AddBezier(
                Gdiplus::PointF(lx + 1.0f, y_mid),
                Gdiplus::PointF(lx + 1.0f, y_mid + 1.5f),
                Gdiplus::PointF(lx + open_w * 0.4f, y_mid),
                Gdiplus::PointF(lx + open_w * 0.4f, y_mid + 6.0f)
            );
            path.AddBezier(
                Gdiplus::PointF(lx + open_w * 0.4f, y_mid + 6.0f),
                Gdiplus::PointF(lx + open_w * 0.4f, y_bot - 6.0f),
                Gdiplus::PointF(lx + open_w * 0.5f, y_bot),
                Gdiplus::PointF(lx + open_w - 2.0f, y_bot)
            );
            g.DrawPath(&pen, &path);
        }

        // Draw inner contents
        float inner_x = lx + (open_w > 0.0f ? open_w + 1.0f : 0.0f);
        if (inner) {
            inner->render(g, inner_x, draw_y + y, brush);
        }

        // Draw close delimiter
        float rx = inner_x + (inner ? inner->width : 0.0f) + (close_w > 0.0f ? 1.0f : 0.0f);
        if (close_delim == L')') {
            Gdiplus::GraphicsPath path;
            path.AddBezier(
                Gdiplus::PointF(rx + 1.0f, y_top),
                Gdiplus::PointF(rx + close_w - 1.0f, y_top + (y_mid - y_top) * 0.45f),
                Gdiplus::PointF(rx + close_w - 1.0f, y_mid + (y_bot - y_mid) * 0.55f),
                Gdiplus::PointF(rx + 1.0f, y_bot)
            );
            g.DrawPath(&pen, &path);
        } else if (close_delim == L']') {
            g.DrawLine(&pen, rx + 1.0f, y_top, rx + close_w - 1.0f, y_top);
            g.DrawLine(&pen, rx + close_w - 1.0f, y_top, rx + close_w - 1.0f, y_bot);
            g.DrawLine(&pen, rx + 1.0f, y_bot, rx + close_w - 1.0f, y_bot);
        } else if (close_delim == L'|') {
            g.DrawLine(&pen, rx + close_w * 0.5f, y_top, rx + close_w * 0.5f, y_bot);
        } else if (close_delim == L'}') {
            Gdiplus::GraphicsPath path;
            path.AddBezier(
                Gdiplus::PointF(rx + 2.0f, y_top),
                Gdiplus::PointF(rx + close_w * 0.5f, y_top),
                Gdiplus::PointF(rx + close_w * 0.6f, y_top + 6.0f),
                Gdiplus::PointF(rx + close_w * 0.6f, y_mid - 6.0f)
            );
            path.AddBezier(
                Gdiplus::PointF(rx + close_w * 0.6f, y_mid - 6.0f),
                Gdiplus::PointF(rx + close_w * 0.6f, y_mid),
                Gdiplus::PointF(rx + close_w - 1.0f, y_mid - 1.5f),
                Gdiplus::PointF(rx + close_w - 1.0f, y_mid)
            );
            path.AddBezier(
                Gdiplus::PointF(rx + close_w - 1.0f, y_mid),
                Gdiplus::PointF(rx + close_w - 1.0f, y_mid + 1.5f),
                Gdiplus::PointF(rx + close_w * 0.6f, y_mid),
                Gdiplus::PointF(rx + close_w * 0.6f, y_mid + 6.0f)
            );
            path.AddBezier(
                Gdiplus::PointF(rx + close_w * 0.6f, y_mid + 6.0f),
                Gdiplus::PointF(rx + close_w * 0.6f, y_bot - 6.0f),
                Gdiplus::PointF(rx + close_w * 0.5f, y_bot),
                Gdiplus::PointF(rx + 2.0f, y_bot)
            );
            g.DrawPath(&pen, &path);
        }
    }
};

struct CasesBox : public LayoutBox {
    std::vector<std::unique_ptr<LayoutBox>> rows;
    float brace_width{18.0f};
    float row_spacing{8.0f};

    void render(Gdiplus::Graphics& g, float draw_x, float draw_y, const Gdiplus::Brush& brush) override {
        const auto* sbrush = dynamic_cast<const Gdiplus::SolidBrush*>(&brush);
        Gdiplus::Color col;
        if (sbrush) sbrush->GetColor(&col);
        else col = Gdiplus::Color(255, 255, 255, 255);

        // Draw elegant curly brace covering total height
        float bx = draw_x + x;
        float by0 = draw_y + y - ascent;
        float by1 = draw_y + y + descent;
        float bym = (by0 + by1) * 0.5f;

        Gdiplus::Pen pen(col, 1.75f);
        pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);

        Gdiplus::GraphicsPath path;
        // Top hook: curving left
        path.AddBezier(
            Gdiplus::PointF(bx + brace_width - 2.0f, by0),
            Gdiplus::PointF(bx + brace_width * 0.5f, by0),
            Gdiplus::PointF(bx + brace_width * 0.4f, by0 + 8.0f),
            Gdiplus::PointF(bx + brace_width * 0.4f, bym - 8.0f)
        );
        // Middle cusp pointing left
        path.AddBezier(
            Gdiplus::PointF(bx + brace_width * 0.4f, bym - 8.0f),
            Gdiplus::PointF(bx + brace_width * 0.4f, bym),
            Gdiplus::PointF(bx + 1.0f, bym - 2.0f),
            Gdiplus::PointF(bx + 1.0f, bym)
        );
        path.AddBezier(
            Gdiplus::PointF(bx + 1.0f, bym),
            Gdiplus::PointF(bx + 1.0f, bym + 2.0f),
            Gdiplus::PointF(bx + brace_width * 0.4f, bym),
            Gdiplus::PointF(bx + brace_width * 0.4f, bym + 8.0f)
        );
        // Bottom hook: curving right
        path.AddBezier(
            Gdiplus::PointF(bx + brace_width * 0.4f, bym + 8.0f),
            Gdiplus::PointF(bx + brace_width * 0.4f, by1 - 8.0f),
            Gdiplus::PointF(bx + brace_width * 0.5f, by1),
            Gdiplus::PointF(bx + brace_width - 2.0f, by1)
        );
        g.DrawPath(&pen, &path);

        // Render each row
        float cur_y = draw_y + y - ascent;
        for (const auto& r : rows) {
            cur_y += r->ascent;
            r->render(g, bx + brace_width + 4.0f, cur_y, brush);
            cur_y += r->descent + row_spacing;
        }
    }
};

struct HBox : public LayoutBox {
    std::vector<std::unique_ptr<LayoutBox>> items;

    void render(Gdiplus::Graphics& g, float draw_x, float draw_y, const Gdiplus::Brush& brush) override {
        for (const auto& it : items) {
            it->render(g, draw_x + x, draw_y + y, brush);
        }
    }
};

struct VBox : public LayoutBox {
    std::vector<std::unique_ptr<LayoutBox>> lines;
    float line_spacing{8.0f};

    void render(Gdiplus::Graphics& g, float draw_x, float draw_y, const Gdiplus::Brush& brush) override {
        float cur_y = draw_y + y;
        for (const auto& line : lines) {
            cur_y += line->ascent;
            line->render(g, draw_x + x, cur_y, brush);
            cur_y += line->descent + line_spacing;
        }
    }
};

// -------------------------------------------------------------
// Math Layout Engine: Font Measurement & TeX Grammar Parser
// -------------------------------------------------------------

class MathLayoutEngine {
public:
    Gdiplus::Graphics& g;
    std::wstring family_name;
    float base_font_size;

    MathLayoutEngine(Gdiplus::Graphics& graphics, const std::string& family, float font_size_pt)
        : g(graphics)
        , base_font_size(font_size_pt * (96.0f / 72.0f)) // Convert points to pixels
    {
        family_name = utf8_to_wide(family);
        if (family_name.empty()) family_name = L"Cambria Math";
    }

    std::unique_ptr<TextRunBox> make_text(
        const std::wstring& str,
        Gdiplus::FontStyle style,
        float font_size,
        float pad_l = 0.0f,
        float pad_r = 0.0f
    ) {
        auto box = std::make_unique<TextRunBox>();
        box->text = str;
        box->style = style;
        box->font_size = font_size;
        box->family_name = family_name;
        box->pad_left = pad_l;
        box->pad_right = pad_r;

        std::wstring actual_family = resolve_font_family(family_name);
        Gdiplus::FontFamily family(actual_family.c_str());
        Gdiplus::Font font(&family, font_size, style, Gdiplus::UnitPixel);
        Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
        format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsMeasureTrailingSpaces);

        Gdiplus::RectF bounds;
        g.MeasureString(str.c_str(), static_cast<int>(str.size()), &font, Gdiplus::PointF(0, 0), &format, &bounds);

        float em = static_cast<float>(family.GetEmHeight(style));
        float cell_asc = static_cast<float>(family.GetCellAscent(style));
        float cell_desc = static_cast<float>(family.GetCellDescent(style));
        float sc = font_size / (em > 0.0f ? em : 2048.0f);

        box->ascent = cell_asc * sc;
        box->descent = cell_desc * sc;
        box->width = bounds.Width + pad_l + pad_r;
        box->height = box->ascent + box->descent;
        return box;
    }

    std::unique_ptr<LayoutBox> parse_full(std::string_view latex_str) {
        std::string s(latex_str);
        // Strip outer delimiters: \[ ... \], $$ ... $$, or $ ... $
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.erase(0, 1);
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.pop_back();

        if (s.starts_with("$$") && s.ends_with("$$") && s.size() >= 4) {
            s = s.substr(2, s.size() - 4);
        } else if (s.starts_with("\\[") && s.ends_with("\\]") && s.size() >= 4) {
            s = s.substr(2, s.size() - 4);
        } else if (s.starts_with("$") && s.ends_with("$") && s.size() >= 2) {
            s = s.substr(1, s.size() - 2);
        }

        // Check for cases environment: \begin{cases} ... \end{cases}
        size_t cases_start = s.find("\\begin{cases}");
        if (cases_start != std::string::npos) {
            size_t cases_end = s.find("\\end{cases}", cases_start);
            if (cases_end != std::string::npos) {
                std::string inside = s.substr(cases_start + 13, cases_end - (cases_start + 13));
                std::string post = s.substr(cases_end + 11);

                auto cases_box = parse_cases(inside, base_font_size);
                if (post.empty()) {
                    return cases_box;
                }

                auto hbox = std::make_unique<HBox>();
                float cx = 0.0f;
                cases_box->x = cx;
                cx += cases_box->width;
                hbox->ascent = cases_box->ascent;
                hbox->descent = cases_box->descent;
                hbox->items.push_back(std::move(cases_box));

                auto post_box = parse_sequence(post, base_font_size);
                post_box->x = cx;
                cx += post_box->width;
                hbox->ascent = std::max(hbox->ascent, post_box->ascent);
                hbox->descent = std::max(hbox->descent, post_box->descent);
                hbox->items.push_back(std::move(post_box));

                hbox->width = cx;
                hbox->height = hbox->ascent + hbox->descent;
                return hbox;
            }
        }

        // Check for aligned environment: \begin{aligned} ... \end{aligned}
        size_t align_start = s.find("\\begin{aligned}");
        if (align_start != std::string::npos) {
            size_t align_end = s.find("\\end{aligned}", align_start);
            if (align_end != std::string::npos) {
                s = s.substr(align_start + 15, align_end - (align_start + 15));
                return parse_multiline(s, base_font_size);
            }
        }

        // Check if multiple lines separated by double backslashes
        if (s.find("\\\\") != std::string::npos) {
            return parse_multiline(s, base_font_size);
        }

        // Standard single expression
        return parse_sequence(s, base_font_size);
    }

    std::unique_ptr<LayoutBox> parse_cases(std::string_view content, float font_size) {
        auto cases_box = std::make_unique<CasesBox>();
        cases_box->brace_width = font_size * 0.75f;
        cases_box->row_spacing = font_size * 0.35f;

        std::vector<std::string> rows;
        size_t cur = 0;
        while (cur < content.size()) {
            size_t nxt = content.find("\\\\", cur);
            if (nxt == std::string::npos) {
                rows.push_back(std::string(content.substr(cur)));
                break;
            }
            rows.push_back(std::string(content.substr(cur, nxt - cur)));
            cur = nxt + 2;
        }

        float max_w = 0.0f;
        float total_h = 0.0f;
        for (const auto& r : rows) {
            auto r_box = parse_sequence(r, font_size);
            max_w = std::max(max_w, r_box->width);
            total_h += r_box->height + cases_box->row_spacing;
            cases_box->rows.push_back(std::move(r_box));
        }
        if (!cases_box->rows.empty()) total_h -= cases_box->row_spacing;

        cases_box->width = cases_box->brace_width + max_w + 6.0f;
        cases_box->height = total_h;
        cases_box->ascent = total_h * 0.5f;
        cases_box->descent = total_h * 0.5f;
        return cases_box;
    }

    std::unique_ptr<LayoutBox> parse_multiline(std::string_view content, float font_size) {
        auto vbox = std::make_unique<VBox>();
        vbox->line_spacing = font_size * 0.35f;

        std::vector<std::string> lines;
        size_t cur = 0;
        while (cur < content.size()) {
            size_t nxt = content.find("\\\\", cur);
            if (nxt == std::string::npos) {
                lines.push_back(std::string(content.substr(cur)));
                break;
            }
            lines.push_back(std::string(content.substr(cur, nxt - cur)));
            cur = nxt + 2;
        }

        float max_w = 0.0f;
        float total_h = 0.0f;
        for (auto& l : lines) {
            // Remove alignment ampersand &= -> =
            size_t amp = l.find("&=");
            if (amp != std::string::npos) l.replace(amp, 2, "=");
            else {
                size_t single_amp = l.find('&');
                if (single_amp != std::string::npos) l.replace(single_amp, 1, " ");
            }

            auto line_box = parse_sequence(l, font_size);
            max_w = std::max(max_w, line_box->width);
            total_h += line_box->height + vbox->line_spacing;
            vbox->lines.push_back(std::move(line_box));
        }
        if (!vbox->lines.empty()) total_h -= vbox->line_spacing;

        vbox->width = max_w;
        vbox->height = total_h;
        vbox->ascent = 0.0f;
        vbox->descent = total_h;
        return vbox;
    }

    std::unique_ptr<LayoutBox> parse_sequence(std::string_view seq, float font_size) {
        auto hbox = std::make_unique<HBox>();
        size_t i = 0;
        const size_t n = seq.size();

        float cur_x = 0.0f;
        float max_asc = font_size * 0.8f;
        float max_desc = font_size * 0.25f;

        auto append_box = [&](std::unique_ptr<LayoutBox> box) {
            box->x = cur_x;
            cur_x += box->width;
            max_asc = std::max(max_asc, box->ascent);
            max_desc = std::max(max_desc, box->descent);
            hbox->items.push_back(std::move(box));
        };

        while (i < n) {
            char c = seq[i];

            // Whitespace
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                ++i;
                continue;
            }

            // Subscript or superscript attachment
            if (c == '^' || c == '_') {
                bool is_sup = (c == '^');
                ++i;
                // Parse script target (single char or braced group)
                std::string script_str;
                if (i < n && seq[i] == '{') {
                    ++i;
                    int depth = 1;
                    while (i < n && depth > 0) {
                        if (seq[i] == '{') ++depth;
                        else if (seq[i] == '}') --depth;
                        if (depth > 0) script_str += seq[i];
                        ++i;
                    }
                } else if (i < n) {
                    script_str += seq[i++];
                }

                float script_font_size = font_size * 0.65f;
                auto script_box = parse_sequence(script_str, script_font_size);

                std::unique_ptr<LayoutBox> base_box;
                if (!hbox->items.empty()) {
                    base_box = std::move(hbox->items.back());
                    hbox->items.pop_back();
                    cur_x = base_box->x;
                    base_box->x = 0.0f;
                    base_box->y = 0.0f;
                }

                // If base is already a SupSubBox, attach without nesting
                if (auto* prev_ss = dynamic_cast<SupSubBox*>(base_box.get())) {
                    if (is_sup && !prev_ss->sup) {
                        prev_ss->sup = std::move(script_box);
                        prev_ss->ascent = std::max(prev_ss->ascent, prev_ss->sup->ascent + prev_ss->sup_shift);
                        prev_ss->width = std::max(prev_ss->width, (prev_ss->base ? prev_ss->base->width : 0.0f) + prev_ss->sup->width + 1.0f);
                        prev_ss->height = prev_ss->ascent + prev_ss->descent;
                        append_box(std::move(base_box));
                        continue;
                    } else if (!is_sup && !prev_ss->sub) {
                        prev_ss->sub = std::move(script_box);
                        prev_ss->descent = std::max(prev_ss->descent, prev_ss->sub->descent + prev_ss->sub_shift);
                        prev_ss->width = std::max(prev_ss->width, (prev_ss->base ? prev_ss->base->width : 0.0f) + prev_ss->sub->width + 1.0f);
                        prev_ss->height = prev_ss->ascent + prev_ss->descent;
                        append_box(std::move(base_box));
                        continue;
                    }
                }

                auto supsub = std::make_unique<SupSubBox>();
                supsub->sup_shift = font_size * 0.42f;
                supsub->sub_shift = font_size * 0.25f;

                float base_w = base_box ? base_box->width : 0.0f;
                supsub->width = base_w + script_box->width + 1.0f;

                if (is_sup) {
                    supsub->ascent = std::max(base_box ? base_box->ascent : font_size * 0.8f, script_box->ascent + supsub->sup_shift);
                    supsub->descent = base_box ? base_box->descent : font_size * 0.25f;
                    supsub->sup = std::move(script_box);
                } else {
                    supsub->ascent = base_box ? base_box->ascent : font_size * 0.8f;
                    supsub->descent = std::max(base_box ? base_box->descent : font_size * 0.25f, script_box->descent + supsub->sub_shift);
                    supsub->sub = std::move(script_box);
                }
                supsub->base = std::move(base_box);
                supsub->height = supsub->ascent + supsub->descent;

                append_box(std::move(supsub));
                continue;
            }

            // LaTeX Commands starting with '\'
            if (c == '\\') {
                ++i;
                std::string cmd;
                while (i < n && ((seq[i] >= 'a' && seq[i] <= 'z') || (seq[i] >= 'A' && seq[i] <= 'Z'))) {
                    cmd += seq[i++];
                }

                if (cmd == "frac") {
                    // Extract numerator {A} and denominator {B}
                    auto read_group = [&]() -> std::string {
                        while (i < n && (seq[i] == ' ' || seq[i] == '\t')) ++i;
                        std::string res;
                        if (i < n && seq[i] == '{') {
                            ++i;
                            int depth = 1;
                            while (i < n && depth > 0) {
                                if (seq[i] == '{') ++depth;
                                else if (seq[i] == '}') --depth;
                                if (depth > 0) res += seq[i];
                                ++i;
                            }
                        } else if (i < n) {
                            res += seq[i++];
                        }
                        return res;
                    };

                    std::string num_str = read_group();
                    std::string den_str = read_group();

                    float sub_size = font_size * 0.85f;
                    auto num_box = parse_sequence(num_str, sub_size);
                    auto den_box = parse_sequence(den_str, sub_size);

                    auto frac = std::make_unique<FracBox>();
                    frac->bar_thickness = std::max(1.5f, font_size * 0.06f);
                    frac->width = std::max(num_box->width, den_box->width) + font_size * 0.4f;
                    float axis_y = -font_size * 0.25f;
                    frac->bar_y = axis_y;
                    frac->ascent = num_box->height + 5.0f - axis_y;
                    frac->descent = den_box->height + 5.0f + axis_y;
                    frac->height = frac->ascent + frac->descent;
                    frac->num = std::move(num_box);
                    frac->den = std::move(den_box);

                    append_box(std::move(frac));
                    continue;
                }

                if (cmd == "sqrt") {
                    // Optional [n] root
                    if (i < n && seq[i] == '[') {
                        while (i < n && seq[i] != ']') ++i;
                        if (i < n && seq[i] == ']') ++i;
                    }
                    std::string rad_str;
                    if (i < n && seq[i] == '{') {
                        ++i;
                        int depth = 1;
                        while (i < n && depth > 0) {
                            if (seq[i] == '{') ++depth;
                            else if (seq[i] == '}') --depth;
                            if (depth > 0) rad_str += seq[i];
                            ++i;
                        }
                    } else if (i < n) {
                        rad_str += seq[i++];
                    }

                    auto rad_box = parse_sequence(rad_str, font_size);
                    auto sqrt_box = std::make_unique<SqrtBox>();
                    sqrt_box->radical_width = font_size * 0.55f;
                    sqrt_box->width = rad_box->width + sqrt_box->radical_width + 4.0f;
                    sqrt_box->ascent = rad_box->ascent + font_size * 0.2f;
                    sqrt_box->descent = rad_box->descent + font_size * 0.08f;
                    sqrt_box->height = sqrt_box->ascent + sqrt_box->descent;
                    sqrt_box->rad = std::move(rad_box);

                    append_box(std::move(sqrt_box));
                    continue;
                }

                if (cmd == "left") {
                    // Skip whitespace after \left
                    while (i < n && (seq[i] == ' ' || seq[i] == '\t')) ++i;

                    // Parse open delimiter
                    wchar_t open_delim = L'(';
                    if (i < n) {
                        if (seq[i] == '\\' && i + 1 < n) {
                            char esc_c = seq[i + 1];
                            open_delim = static_cast<wchar_t>(esc_c);
                            i += 2;
                        } else {
                            open_delim = static_cast<wchar_t>(seq[i++]);
                        }
                    }

                    // Scan forward to matching \right, respecting nesting
                    int left_depth = 1;
                    std::string inner_str;
                    while (i < n && left_depth > 0) {
                        if (seq[i] == '\\') {
                            if (seq.compare(i, 5, "\\left") == 0 && (i + 5 >= n || !isalpha(static_cast<unsigned char>(seq[i + 5])))) {
                                left_depth++;
                                inner_str += "\\left";
                                i += 5;
                                continue;
                            } else if (seq.compare(i, 6, "\\right") == 0 && (i + 6 >= n || !isalpha(static_cast<unsigned char>(seq[i + 6])))) {
                                left_depth--;
                                if (left_depth == 0) {
                                    i += 6;
                                    break;
                                }
                                inner_str += "\\right";
                                i += 6;
                                continue;
                            }
                        }
                        inner_str += seq[i++];
                    }

                    // Parse close delimiter
                    while (i < n && (seq[i] == ' ' || seq[i] == '\t')) ++i;
                    wchar_t close_delim = L')';
                    if (i < n) {
                        if (seq[i] == '\\' && i + 1 < n) {
                            char esc_c = seq[i + 1];
                            close_delim = static_cast<wchar_t>(esc_c);
                            i += 2;
                        } else {
                            close_delim = static_cast<wchar_t>(seq[i++]);
                        }
                    }

                    auto inner_box = parse_sequence(inner_str, font_size);
                    auto dbox = std::make_unique<DelimitedBox>();
                    dbox->open_delim = open_delim;
                    dbox->close_delim = close_delim;

                    float inner_h = inner_box ? inner_box->height : font_size;
                    dbox->delim_width = std::max(font_size * 0.28f, std::min(font_size * 0.5f, inner_h * 0.18f));

                    float open_w = (open_delim != L'.' && open_delim != 0) ? dbox->delim_width : 0.0f;
                    float close_w = (close_delim != L'.' && close_delim != 0) ? dbox->delim_width : 0.0f;

                    float axis = -font_size * 0.25f;
                    float in_asc = inner_box ? inner_box->ascent : font_size * 0.8f;
                    float in_desc = inner_box ? inner_box->descent : font_size * 0.25f;
                    float dist_top = in_asc + axis;
                    float dist_bot = in_desc - axis;
                    float max_half = std::max(dist_top, dist_bot) + 2.0f;

                    dbox->ascent = max_half - axis;
                    dbox->descent = max_half + axis;
                    dbox->height = dbox->ascent + dbox->descent;

                    dbox->width = open_w + (open_w > 0.0f ? 1.0f : 0.0f) + (inner_box ? inner_box->width : 0.0f) + (close_w > 0.0f ? 1.0f : 0.0f) + close_w;
                    dbox->inner = std::move(inner_box);

                    append_box(std::move(dbox));
                    continue;
                }

                if (cmd == "right") {
                    continue;
                }

                if (cmd == "quad") {
                    float qw = font_size * 1.0f;
                    auto space_box = make_text(L" ", Gdiplus::FontStyleRegular, font_size);
                    space_box->width = qw;
                    append_box(std::move(space_box));
                    continue;
                }

                if (cmd == "qquad") {
                    float qw = font_size * 2.0f;
                    auto space_box = make_text(L" ", Gdiplus::FontStyleRegular, font_size);
                    space_box->width = qw;
                    append_box(std::move(space_box));
                    continue;
                }

                if (cmd == "text" || cmd == "operatorname" || cmd == "mathrm" || cmd == "mathbf") {
                    std::string txt;
                    if (i < n && seq[i] == '{') {
                        ++i;
                        int depth = 1;
                        while (i < n && depth > 0) {
                            if (seq[i] == '{') ++depth;
                            else if (seq[i] == '}') --depth;
                            if (depth > 0) txt += seq[i];
                            ++i;
                        }
                    }
                    std::string clean_txt;
                    for (size_t k = 0; k < txt.size(); ++k) {
                        if (txt[k] == '\\' && k + 1 < txt.size()) {
                            char nc = txt[k + 1];
                            if (nc == '_' || nc == '{' || nc == '}' || nc == '%' || nc == '&' || nc == '$' || nc == '#') {
                                clean_txt += nc;
                                ++k;
                                continue;
                            }
                        }
                        clean_txt += txt[k];
                    }
                    auto style = (cmd == "mathbf") ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular;
                    auto box = make_text(utf8_to_wide(clean_txt), style, font_size, 1.0f, 1.0f);
                    append_box(std::move(box));
                    continue;
                }

                // Mathematical symbols & Greek letters mapping
                static const std::vector<std::pair<std::string, std::wstring>> symbols = {
                    {"alpha", L"\u03B1"}, {"beta", L"\u03B2"}, {"gamma", L"\u03B3"},
                    {"delta", L"\u03B4"}, {"epsilon", L"\u03B5"}, {"theta", L"\u03B8"},
                    {"nu", L"\u03BD"}, {"pi", L"\u03C0"}, {"omega", L"\u03C9"},
                    {"lambda", L"\u03BB"}, {"sigma", L"\u03C3"}, {"mu", L"\u03BC"},
                    {"Gamma", L"\u0393"}, {"Delta", L"\u0394"}, {"Theta", L"\u0398"},
                    {"Lambda", L"\u039B"}, {"Pi", L"\u03A0"}, {"Sigma", L"\u03A3"},
                    {"Omega", L"\u03A9"}, {"cdot", L"\u22C5"}, {"times", L"\u00D7"},
                    {"div", L"\u00F7"}, {"pm", L"\u00B1"}, {"mp", L"\u2213"},
                    {"leq", L"\u2264"}, {"geq", L"\u2265"}, {"neq", L"\u2260"},
                    {"approx", L"\u2248"}, {"in", L"\u2208"}, {"notin", L"\u2209"},
                    {"infty", L"\u221E"}, {"partial", L"\u2202"}
                };

                bool found_symbol = false;
                for (const auto& [sym, wch] : symbols) {
                    if (cmd == sym) {
                        bool is_greek_lower = (cmd != "Delta" && cmd != "Omega" && cmd != "Pi" && cmd != "Sigma" &&
                                              cmd != "cdot" && cmd != "times" && cmd != "div" && cmd != "pm" &&
                                              cmd != "mp" && cmd != "leq" && cmd != "geq" && cmd != "neq" &&
                                              cmd != "approx" && cmd != "in" && cmd != "notin" && cmd != "infty");
                        Gdiplus::FontStyle sym_style = is_greek_lower ? Gdiplus::FontStyleItalic : Gdiplus::FontStyleRegular;
                        float pad = (cmd == "cdot" || cmd == "times" || cmd == "pm" || cmd == "leq" || cmd == "geq" || cmd == "neq" || cmd == "in") ? font_size * 0.22f : 1.0f;
                        append_box(make_text(wch, sym_style, font_size, pad, pad));
                        found_symbol = true;
                        break;
                    }
                }
                if (found_symbol) continue;

                // Named standard functions (sin, cos, tan, exp, ln, log, etc.)
                static const std::vector<std::string> funcs = {
                    "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh",
                    "exp", "ln", "log", "sec", "csc", "cot"
                };
                bool found_func = false;
                for (const auto& f : funcs) {
                    if (cmd == f) {
                        append_box(make_text(utf8_to_wide(f), Gdiplus::FontStyleRegular, font_size, 0.0f, font_size * 0.12f));
                        found_func = true;
                        break;
                    }
                }
                if (found_func) continue;

                // Escaped characters: \{, \}, \_, \%, \&
                if (cmd.empty() && i < n) {
                    char next_esc = seq[i++];
                    wchar_t wesc[2] = {static_cast<wchar_t>(next_esc), 0};
                    append_box(make_text(wesc, Gdiplus::FontStyleRegular, font_size));
                    continue;
                }

                // General command fallback as text
                append_box(make_text(utf8_to_wide(cmd), Gdiplus::FontStyleRegular, font_size));
                continue;
            }

            // Braced block { ... }
            if (c == '{') {
                ++i;
                int depth = 1;
                std::string inside;
                while (i < n && depth > 0) {
                    if (seq[i] == '{') ++depth;
                    else if (seq[i] == '}') --depth;
                    if (depth > 0) inside += seq[i];
                    ++i;
                }
                auto group_box = parse_sequence(inside, font_size);
                append_box(std::move(group_box));
                continue;
            }

            // Numbers: 0..9 and decimal point '.'
            if ((c >= '0' && c <= '9') || (c == '.' && i + 1 < n && seq[i + 1] >= '0' && seq[i + 1] <= '9')) {
                std::wstring num_str;
                while (i < n && ((seq[i] >= '0' && seq[i] <= '9') || seq[i] == '.')) {
                    num_str += static_cast<wchar_t>(seq[i++]);
                }
                append_box(make_text(num_str, Gdiplus::FontStyleRegular, font_size));
                continue;
            }

            // Mathematical minus sign: '-' in math mode should be Unicode \u2212
            if (c == '-') {
                ++i;
                float pad_l = font_size * 0.22f;
                float pad_r = font_size * 0.22f;
                // If at beginning or immediately following an open delimiter or relation, treat as unary minus
                if (hbox->items.empty()) {
                    pad_l = 0.0f;
                    pad_r = 1.0f;
                } else if (auto* tr = dynamic_cast<TextRunBox*>(hbox->items.back().get())) {
                    if (tr->text == L"(" || tr->text == L"[" || tr->text == L"=" || tr->text == L"<" || tr->text == L">") {
                        pad_l = 0.0f;
                        pad_r = 1.0f;
                    }
                }
                append_box(make_text(L"\u2212", Gdiplus::FontStyleRegular, font_size, pad_l, pad_r));
                continue;
            }

            // Binary & relational operators: '+', '=', '<', '>', '*'
            if (c == '+' || c == '=' || c == '<' || c == '>' || c == '*') {
                ++i;
                float pad = (c == '=') ? font_size * 0.28f : font_size * 0.22f;
                wchar_t op_str[2] = {static_cast<wchar_t>(c == '*' ? L'\u22C5' : c), 0};
                append_box(make_text(op_str, Gdiplus::FontStyleRegular, font_size, pad, pad));
                continue;
            }

            // Delimiters & punctuation: '(', ')', '[', ']', ',', ';', '!'
            if (c == '(' || c == ')' || c == '[' || c == ']' || c == ',' || c == ';' || c == '!' || c == '|') {
                ++i;
                float pad_r = (c == ',' || c == ';') ? font_size * 0.22f : 0.0f;
                wchar_t p_str[2] = {static_cast<wchar_t>(c), 0};
                append_box(make_text(p_str, Gdiplus::FontStyleRegular, font_size, 0.0f, pad_r));
                continue;
            }

            // Single letters: variables (x, y, a, b, etc.) -> Italic font
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                // Check if multichar identifier like "diff_step"
                std::string word;
                while (i < n && ((seq[i] >= 'a' && seq[i] <= 'z') || (seq[i] >= 'A' && seq[i] <= 'Z') || seq[i] == '_')) {
                    word += seq[i++];
                }
                if (word.size() == 1) {
                    wchar_t wch[2] = {static_cast<wchar_t>(word[0]), 0};
                    float pad_l = 0.5f;
                    float pad_r = 0.5f;
                    append_box(make_text(wch, Gdiplus::FontStyleItalic, font_size, pad_l, pad_r));
                } else {
                    // Check named functions without backslash (e.g. sin, cos)
                    if (word == "sin" || word == "cos" || word == "tan" || word == "exp" || word == "ln" || word == "diff_step" || word == "hypot" || word == "atan2") {
                        append_box(make_text(utf8_to_wide(word), Gdiplus::FontStyleRegular, font_size, 0.0f, font_size * 0.12f));
                    } else {
                        append_box(make_text(utf8_to_wide(word), Gdiplus::FontStyleItalic, font_size, 0.5f, 0.5f));
                    }
                }
                continue;
            }

            // Any remaining characters
            wchar_t wch[2] = {static_cast<wchar_t>(c), 0};
            append_box(make_text(wch, Gdiplus::FontStyleRegular, font_size));
            ++i;
        }

        hbox->width = cur_x;
        hbox->ascent = max_asc;
        hbox->descent = max_desc;
        hbox->height = max_asc + max_desc;
        return hbox;
    }
};

#endif // _WIN32

} // namespace

FrameBuffer LatexRenderModule::render_math_to_framebuffer(
    std::string_view latex_text,
    Color text_color,
    Color bg_color,
    float font_size_pt,
    const std::string& font_family
) {
#ifdef _WIN32
    if (!GdiplusManager::init() || latex_text.empty()) {
        return FrameBuffer(10, 10, bg_color);
    }

    // Step 1: Measure bounds using a temporary 1x1 bitmap
    Gdiplus::Bitmap measure_bmp(1, 1, PixelFormat32bppARGB);
    Gdiplus::Graphics measure_g(&measure_bmp);
    measure_g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    MathLayoutEngine engine(measure_g, font_family, font_size_pt);
    auto root_box = engine.parse_full(latex_text);

    int margin_x = static_cast<int>(font_size_pt * 0.5f);
    int margin_y = static_cast<int>(font_size_pt * 0.35f);
    int width = static_cast<int>(std::ceil(root_box->width)) + margin_x * 2;
    int height = static_cast<int>(std::ceil(root_box->height)) + margin_y * 2;

    width = std::max(width, 16);
    height = std::max(height, 16);

    // Step 2: Render to high-quality 32-bit ARGB bitmap
    Gdiplus::Bitmap bmp(width, height, PixelFormat32bppARGB);
    Gdiplus::Graphics g(&bmp);

    g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    if (bg_color.a == 0) {
        g.Clear(Gdiplus::Color(0, 0, 0, 0));
    } else {
        g.Clear(Gdiplus::Color(bg_color.a, bg_color.r, bg_color.g, bg_color.b));
    }

    Gdiplus::SolidBrush text_brush(Gdiplus::Color(text_color.a, text_color.r, text_color.g, text_color.b));
    float baseline_y = margin_y + root_box->ascent;
    root_box->render(g, static_cast<float>(margin_x), baseline_y, text_brush);

    // Step 3: Copy pixel data to Formulaic FrameBuffer
    FrameBuffer fb(width, height, bg_color);
    Gdiplus::BitmapData data;
    Gdiplus::Rect rect(0, 0, width, height);

    if (bmp.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) == Gdiplus::Ok) {
        auto* src_row = reinterpret_cast<const uint8_t*>(data.Scan0);
        uint32_t* dst_pixels = fb.data();

        for (int y = 0; y < height; ++y) {
            const auto* src_pixels = reinterpret_cast<const uint32_t*>(src_row);
            std::memcpy(dst_pixels + static_cast<size_t>(y) * width, src_pixels, static_cast<size_t>(width) * sizeof(uint32_t));
            src_row += data.Stride;
        }
        bmp.UnlockBits(&data);
    }

    return fb;
#else
    FrameBuffer fb(200, 50, bg_color);
    fb.draw_text(10, 15, std::string(latex_text), text_color, 1);
    return fb;
#endif
}

bool LatexRenderModule::export_math_image(
    std::string_view latex_text,
    const std::string& output_filepath,
    Color text_color,
    Color bg_color,
    float font_size_pt,
    const std::string& font_family
) {
#ifdef _WIN32
    if (!GdiplusManager::init() || latex_text.empty()) return false;

    // Render using our math layout engine
    FrameBuffer fb = render_math_to_framebuffer(latex_text, text_color, bg_color, font_size_pt, font_family);
    if (fb.width() <= 0 || fb.height() <= 0) return false;

    // Create GDI+ bitmap from FrameBuffer BGRA pixels
    Gdiplus::Bitmap bmp(fb.width(), fb.height(), static_cast<INT>(fb.stride_bytes()), PixelFormat32bppARGB, reinterpret_cast<BYTE*>(fb.data()));

    // Determine encoder by file extension
    std::wstring out_w = utf8_to_wide(output_filepath);
    CLSID clsid;
    if (output_filepath.ends_with(".png") || output_filepath.ends_with(".PNG")) {
        get_image_encoder_clsid(L"image/png", &clsid);
    } else if (output_filepath.ends_with(".bmp") || output_filepath.ends_with(".BMP")) {
        get_image_encoder_clsid(L"image/bmp", &clsid);
    } else {
        get_image_encoder_clsid(L"image/png", &clsid);
    }

    auto status = bmp.Save(out_w.c_str(), &clsid, nullptr);
    return (status == Gdiplus::Ok);
#else
    return false;
#endif
}

void LatexRenderModule::render_card(
    FrameBuffer& fb,
    int x,
    int y,
    std::string_view latex_text,
    const LatexRenderStyle& style
) {
    if (latex_text.empty()) return;

#ifdef _WIN32
    // High-resolution LaTeXLive vector mathematical rendering
    FrameBuffer math_fb = render_math_to_framebuffer(
        latex_text,
        style.text_color,
        Color::Transparent,
        style.font_size_pt,
        style.font_family
    );

    if (math_fb.width() > 0 && math_fb.height() > 0) {
        const int pad_x = 14;
        const int pad_y = 10;
        const int title_h = 16;
        const int card_w = math_fb.width() + pad_x * 2;
        const int card_h = math_fb.height() + pad_y * 2 + title_h;

        int draw_x = std::clamp(x, 4, std::max(4, fb.width() - card_w - 4));
        int draw_y = std::clamp(y, 4, std::max(4, fb.height() - card_h - 4));

        // Draw card container
        fb.fill_rounded_rect(draw_x, draw_y, card_w, card_h, 8, style.background_color);
        fb.draw_rounded_rect(draw_x, draw_y, card_w, card_h, 8, style.border_color, 1.5);

        // Header watermark tag
        fb.draw_text(draw_x + pad_x, draw_y + pad_y, "[ LaTeXLive Formula ]", style.title_color, 1);

        // Alpha-blend mathematical formula pixels onto the card
        int math_ox = draw_x + pad_x;
        int math_oy = draw_y + pad_y + title_h + 2;

        for (int my = 0; my < math_fb.height(); ++my) {
            int target_y = math_oy + my;
            if (target_y < 0 || target_y >= fb.height()) continue;

            for (int mx = 0; mx < math_fb.width(); ++mx) {
                int target_x = math_ox + mx;
                if (target_x < 0 || target_x >= fb.width()) continue;

                Color px = math_fb.get_pixel(mx, my);
                if (px.a > 0) {
                    fb.blend_pixel(target_x, target_y, px, BlendMode::AlphaBlend);
                }
            }
        }
        return;
    }
#endif

    // Portable fallback
    fb.fill_rounded_rect(x, y, 240, 60, 6, style.background_color);
    fb.draw_rounded_rect(x, y, 240, 60, 6, style.border_color, 1.5);
    fb.draw_text(x + 10, y + 10, "[ LaTeXLive Formula ]", style.title_color, 1);
    fb.draw_text(x + 10, y + 28, std::string(latex_text).substr(0, 32), style.text_color, 1);
}

void LatexRenderModule::render_full(
    FrameBuffer& fb,
    const Viewport& vp,
    std::string_view latex_text,
    const LatexRenderStyle& style,
    double time_t
) {
    render_latex_plot(fb, vp, latex_text, style.curve_color, style.line_thickness, time_t);
    if (style.show_card) {
        render_card(fb, style.card_margin_x, style.card_margin_y, latex_text, style);
    }
}

} // namespace formulaic
