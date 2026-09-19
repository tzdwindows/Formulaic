#pragma once

#include <Formulaic/core/error.hpp>
#include <Formulaic/core/export.hpp>
#include <Formulaic/parser/expression.hpp>
#include <Formulaic/parser/latex_converter.hpp>
#include <Formulaic/render/color.hpp>
#include <Formulaic/render/framebuffer.hpp>
#include <Formulaic/render/viewport.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace formulaic {

struct FORMULAIC_API LatexRenderStyle {
    Color text_color{Color::White};
    Color title_color{Color::Cyan};
    Color background_color{Color(18, 22, 34, 230)};
    Color border_color{Color(65, 80, 130, 255)};
    Color curve_color{Color::NeonPink};
    double line_thickness{2.0};
    bool show_card{true};
    int card_margin_x{20};
    int card_margin_y{20};
    int font_scale{1};
    float font_size_pt{22.0f};
    std::string font_family{"Cambria Math"};

    LatexRenderStyle() = default;
};

class FORMULAIC_API LatexRenderModule {
public:
    // Compile a standard LaTeX formula directly into an executable Expression
    [[nodiscard]] static Result<Expression> compile_latex(
        std::string_view latex_text,
        const std::vector<std::string>& variable_names = {"x", "y", "t", "r", "u", "v", "h"}
    );

    // Renders the mathematical plot corresponding to the LaTeX formula onto the FrameBuffer
    static void render_latex_plot(
        FrameBuffer& fb,
        const Viewport& vp,
        std::string_view latex_text,
        Color color = Color::NeonPink,
        double line_thickness = 2.0,
        double time_t = 0.0
    );

    // Renders a visual typeset LaTeXLive mathematical formula banner card on the FrameBuffer
    static void render_card(
        FrameBuffer& fb,
        int x,
        int y,
        std::string_view latex_text,
        const LatexRenderStyle& style = LatexRenderStyle()
    );

    // Renders a high-resolution mathematical formula directly to a new FrameBuffer matching LaTeXLive style
    [[nodiscard]] static FrameBuffer render_math_to_framebuffer(
        std::string_view latex_text,
        Color text_color = Color::Black,
        Color bg_color = Color::Transparent,
        float font_size_pt = 36.0f,
        const std::string& font_family = "Cambria Math"
    );

    // Export a typeset formula image (PNG / BMP) matching LaTeXLive style
    static bool export_math_image(
        std::string_view latex_text,
        const std::string& output_filepath,
        Color text_color = Color::Black,
        Color bg_color = Color::Transparent,
        float font_size_pt = 48.0f,
        const std::string& font_family = "Cambria Math"
    );

    // Combined rendering: plots curve/field and superimposes the visual formula banner card
    static void render_full(
        FrameBuffer& fb,
        const Viewport& vp,
        std::string_view latex_text,
        const LatexRenderStyle& style = LatexRenderStyle(),
        double time_t = 0.0
    );
};

} // namespace formulaic
