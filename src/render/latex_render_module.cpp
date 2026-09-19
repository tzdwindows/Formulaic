#include <Formulaic/render/latex_render_module.hpp>
#include <Formulaic/render/raster_engine.hpp>
#include <algorithm>
#include <sstream>

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

    // Check if implicit equation or explicit curve
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

std::string typeset_latex_for_display(std::string_view line) {
    std::string s(line);
    auto replace_all = [](std::string& str, std::string_view from, std::string_view to) {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    // Convert \frac{A}{B} to (A)/(B)
    while (true) {
        size_t pos = s.find("\\frac{");
        if (pos == std::string::npos) break;
        size_t num_start = pos + 6;
        int depth = 1;
        size_t num_end = num_start;
        while (num_end < s.size() && depth > 0) {
            if (s[num_end] == '{') ++depth;
            else if (s[num_end] == '}') --depth;
            ++num_end;
        }
        if (depth != 0 || num_end >= s.size() || s[num_end] != '{') {
            replace_all(s, "\\frac{", "(");
            break;
        }
        std::string num = s.substr(num_start, num_end - 1 - num_start);
        size_t den_start = num_end + 1;
        depth = 1;
        size_t den_end = den_start;
        while (den_end < s.size() && depth > 0) {
            if (s[den_end] == '{') ++depth;
            else if (s[den_end] == '}') --depth;
            ++den_end;
        }
        if (depth != 0) {
            replace_all(s, "\\frac{", "(");
            break;
        }
        std::string den = s.substr(den_start, den_end - 1 - den_start);
        std::string repl = "(" + num + ")/(" + den + ")";
        s.replace(pos, den_end - pos, repl);
    }

    replace_all(s, "&=", "=");
    replace_all(s, "&", " ");
    replace_all(s, "\\cdot", "*");
    replace_all(s, "\\times", "*");
    replace_all(s, "\\div", "/");
    replace_all(s, "\\pm", "+/-");
    replace_all(s, "\\leq", "<=");
    replace_all(s, "\\geq", ">=");
    replace_all(s, "\\neq", "!=");
    replace_all(s, "\\left(", "(");
    replace_all(s, "\\right)", ")");
    replace_all(s, "\\left[", "[");
    replace_all(s, "\\right]", "]");
    replace_all(s, "\\left|", "|");
    replace_all(s, "\\right|", "|");
    replace_all(s, "\\operatorname{diff\\_step}", "diff_step");
    replace_all(s, "\\operatorname{", "");
    replace_all(s, "\\text{", "");
    replace_all(s, "\\mathrm{", "");
    replace_all(s, "\\sqrt{", "sqrt(");
    replace_all(s, "\\sin", "sin");
    replace_all(s, "\\cos", "cos");
    replace_all(s, "\\tan", "tan");
    replace_all(s, "\\asin", "asin");
    replace_all(s, "\\acos", "acos");
    replace_all(s, "\\atan", "atan");
    replace_all(s, "\\exp", "exp");
    replace_all(s, "\\ln", "ln");
    replace_all(s, "\\log", "log");
    replace_all(s, "\\alpha", "alpha");
    replace_all(s, "\\beta", "beta");
    replace_all(s, "\\gamma", "gamma");
    replace_all(s, "\\delta", "delta");
    replace_all(s, "\\theta", "theta");
    replace_all(s, "\\nu", "nu");
    replace_all(s, "\\pi", "pi");
    replace_all(s, "\\omega", "omega");
    replace_all(s, "\\_", "_");
    replace_all(s, "^{2}", "^2");
    replace_all(s, "^{3}", "^3");
    replace_all(s, "^{", "^(");
    replace_all(s, "_{0}", "0");
    replace_all(s, "_{1}", "1");
    replace_all(s, "_{2}", "2");
    replace_all(s, "_{", "_");

    // Remove isolated braces
    std::string clean;
    clean.reserve(s.size());
    for (char c : s) {
        if (c != '{' && c != '}') clean += c;
    }
    return clean;
}

} // namespace

void LatexRenderModule::render_card(
    FrameBuffer& fb,
    int x,
    int y,
    std::string_view latex_text,
    const LatexRenderStyle& style
) {
    if (latex_text.empty()) return;

    std::string s(latex_text);
    // Strip \[ ... \] or \begin{aligned} ... \end{aligned}
    if (s.starts_with("\\[") && s.ends_with("\\]")) {
        s = s.substr(2, s.size() - 4);
    }
    size_t env_start = s.find("\\begin{aligned}");
    size_t env_end = s.rfind("\\end{aligned}");
    if (env_start != std::string::npos && env_end != std::string::npos && env_end > env_start) {
        s = s.substr(env_start + 15, env_end - (env_start + 15));
    }

    // Split into display lines
    std::vector<std::string> lines;
    size_t cur = 0;
    while (cur < s.size()) {
        size_t next_dslash = s.find("\\\\", cur);
        std::string raw_line;
        if (next_dslash == std::string::npos) {
            raw_line = s.substr(cur);
            cur = s.size();
        } else {
            raw_line = s.substr(cur, next_dslash - cur);
            cur = next_dslash + 2;
        }
        while (!raw_line.empty() && (raw_line.front() == ' ' || raw_line.front() == '\t' || raw_line.front() == '\r' || raw_line.front() == '\n')) raw_line.erase(0, 1);
        while (!raw_line.empty() && (raw_line.back() == ' ' || raw_line.back() == '\t' || raw_line.back() == '\r' || raw_line.back() == '\n')) raw_line.pop_back();
        if (!raw_line.empty()) {
            lines.push_back(typeset_latex_for_display(raw_line));
        }
    }

    if (lines.empty()) return;

    // Limit displayed lines on card if too long
    if (lines.size() > 6) {
        std::vector<std::string> truncated;
        for (size_t k = 0; k < 4; ++k) truncated.push_back(lines[k]);
        truncated.push_back("... (" + std::to_string(lines.size() - 5) + " more statements) ...");
        truncated.push_back(lines.back());
        lines = std::move(truncated);
    }

    size_t max_len = 16; // Minimum width for header
    for (const auto& l : lines) {
        max_len = std::max(max_len, l.size());
    }

    const int scale = std::clamp(style.font_scale, 1, 2);
    const int char_w = 6 * scale;
    const int line_h = 13 * scale;
    const int pad_x = 12;
    const int pad_y = 10;
    const int card_w = static_cast<int>(max_len) * char_w + pad_x * 2;
    const int card_h = static_cast<int>(lines.size() + 1) * line_h + pad_y * 2 + 4;

    // Ensure within screen bounds
    int draw_x = std::clamp(x, 4, std::max(4, fb.width() - card_w - 4));
    int draw_y = std::clamp(y, 4, std::max(4, fb.height() - card_h - 4));

    // Draw card background
    fb.fill_rounded_rect(draw_x, draw_y, card_w, card_h, 6, style.background_color);
    fb.draw_rounded_rect(draw_x, draw_y, card_w, card_h, 6, style.border_color, 1.5);

    // Draw header tag
    fb.draw_text(draw_x + pad_x, draw_y + pad_y, "[ LaTeXLive Formula ]", style.title_color, scale);

    // Draw typeset lines
    int text_y = draw_y + pad_y + line_h + 4;
    for (const auto& l : lines) {
        fb.draw_text(draw_x + pad_x, text_y, l, style.text_color, scale);
        text_y += line_h;
    }
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
