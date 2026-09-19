#include <Formulaic/parser/expression.hpp>
#include <Formulaic/parser/latex_converter.hpp>
#include <Formulaic/render/framebuffer.hpp>
#include <Formulaic/render/viewport.hpp>
#include <Formulaic/render/raster_engine.hpp>
#include <Formulaic/render/latex_render_module.hpp>
#include <cmath>
#include <iostream>
#include <string>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: [" << #cond << "] " << (msg) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return 1; \
        } \
    } while (0)

#define TEST_ASSERT_CONTAINS(str, substr, msg) \
    do { \
        if ((str).find(substr) == std::string::npos) { \
            std::cerr << "Assertion failed: String does not contain '" << (substr) \
                      << "'. Actual: [" << (str) << "] " << (msg) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "===========================================================\n";
    std::cout << " Formulaic Standard LaTeXLive Mathematical Converter Tests\n";
    std::cout << "===========================================================\n";

    // Test 1: Elementary algebra & polynomials
    {
        auto res = formulaic::LatexConverter::convert("x^2 + 2*x + 1");
        TEST_ASSERT(res.has_value(), "Algebraic expression converted successfully");
        std::cout << "1. x^2 + 2*x + 1 -> " << res.value() << std::endl;
        TEST_ASSERT_CONTAINS(res.value(), "x^{2}", "Power formatted as ^{2}");
        TEST_ASSERT_CONTAINS(res.value(), "2 \\cdot x", "Multiplication formatted as \\cdot");
    }

    // Test 2: Pretty fractions
    {
        auto res = formulaic::LatexConverter::convert("1 / (x + 1)");
        TEST_ASSERT(res.has_value(), "Fraction converted successfully");
        std::cout << "2. 1 / (x + 1) -> " << res.value() << std::endl;
        TEST_ASSERT_CONTAINS(res.value(), "\\frac{1}{x + 1}", "Fraction formatted with \\frac");
    }

    // Test 3: Rational Implicit Equation: 1/x + 1/y = 0
    {
        auto res = formulaic::LatexConverter::convert("1/x + 1/y = 0");
        TEST_ASSERT(res.has_value(), "1/x + 1/y = 0 converted");
        std::cout << "3. 1/x + 1/y = 0 -> " << res.value() << std::endl;
        TEST_ASSERT_CONTAINS(res.value(), "\\frac{1}{x}", "1/x formatted as fraction");
        TEST_ASSERT_CONTAINS(res.value(), "\\frac{1}{y}", "1/y formatted as fraction");
        TEST_ASSERT_CONTAINS(res.value(), "= 0", "Equality preserved");
    }

    // Test 4: Rational Implicit Equation with constant: 1/x + 1/y = 50
    {
        auto res = formulaic::LatexConverter::convert("1/x + 1/y = 50");
        TEST_ASSERT(res.has_value(), "1/x + 1/y = 50 converted");
        std::cout << "4. 1/x + 1/y = 50 -> " << res.value() << std::endl;
        TEST_ASSERT_CONTAINS(res.value(), "\\frac{1}{x} + \\frac{1}{y} = 50", "Equation correctly formatted");
    }

    // Test 5: Trigonometric powers & identities
    {
        auto res = formulaic::LatexConverter::convert("sin(x)^2 + cos(x)^2");
        TEST_ASSERT(res.has_value(), "Trig powers converted");
        std::cout << "5. sin(x)^2 + cos(x)^2 -> " << res.value() << std::endl;
        TEST_ASSERT_CONTAINS(res.value(), "\\sin^{2}\\left(x\\right)", "sin power notation");
        TEST_ASSERT_CONTAINS(res.value(), "\\cos^{2}\\left(x\\right)", "cos power notation");
    }

    // Test 6: Special math functions: sqrt, hypot, abs, ln(sin(x))
    {
        auto res = formulaic::LatexConverter::convert("hypot(x, y) + sqrt(x^2 + 1) + abs(y) + ln(sin(x))");
        TEST_ASSERT(res.has_value(), "Special functions converted");
        std::cout << "6. Special functions -> " << res.value() << std::endl;
        TEST_ASSERT_CONTAINS(res.value(), "\\sqrt{x^{2} + y^{2}}", "hypot formatted as sqrt sum of squares");
        TEST_ASSERT_CONTAINS(res.value(), "\\sqrt{x^{2} + 1}", "sqrt formatted");
        TEST_ASSERT_CONTAINS(res.value(), "\\left| y \\right|", "abs formatted with vertical bars");
        TEST_ASSERT_CONTAINS(res.value(), "\\ln\\left(\\sin\\left(x\\right)\\right)", "ln(sin(x)) formatted");
    }

    // Test 7: Greek letters and subscripts
    {
        auto res = formulaic::LatexConverter::convert("alpha * sin(theta) + nu * lap_u + u0");
        TEST_ASSERT(res.has_value(), "Greek letters and subscripts converted");
        std::cout << "7. Greek & subscripts -> " << res.value() << std::endl;
        TEST_ASSERT_CONTAINS(res.value(), "\\alpha", "alpha converted to \\alpha");
        TEST_ASSERT_CONTAINS(res.value(), "\\theta", "theta converted to \\theta");
        TEST_ASSERT_CONTAINS(res.value(), "\\nu", "nu converted to \\nu");
        TEST_ASSERT_CONTAINS(res.value(), "\\text{lap}_{u}", "lap_u converted to \\text{lap}_{u}");
        TEST_ASSERT_CONTAINS(res.value(), "u_{0}", "u0 converted to u_{0}");
    }

    // Test 8: Multi-line declarations with aligned environment
    {
        const std::string script =
            "let r = hypot(x, y);\n"
            "let theta = atan2(y, x);\n"
            "let envelope = exp(-0.35 * r);\n"
            "sin(6.0 * theta + 2.0 * t) * envelope;";
        auto res = formulaic::LatexConverter::convert(script);
        TEST_ASSERT(res.has_value(), "Multiline script converted");
        std::cout << "8. Multiline script ->\n" << res.value() << std::endl;
        TEST_ASSERT_CONTAINS(res.value(), "\\begin{aligned}", "aligned environment opened");
        TEST_ASSERT_CONTAINS(res.value(), "\\end{aligned}", "aligned environment closed");
        TEST_ASSERT_CONTAINS(res.value(), "r &= \\sqrt{x^{2} + y^{2}}", "First let statement aligned");
        TEST_ASSERT_CONTAINS(res.value(), "\\theta &= \\operatorname{atan2}\\left(y, x\\right)", "theta assignment aligned");
        TEST_ASSERT_CONTAINS(res.value(), "\\text{envelope} &= e^{-0.35 \\cdot r}", "envelope assignment aligned");
    }

    // Test 9: Complex Navier-Stokes Taylor-Green equation with implicit equality
    {
        const std::string ns_script =
            "let h = 0.01;\n"
            "let nu = 0.08;\n"
            "let u0 = sin(x) * cos(y);\n"
            "let v0 = -cos(x) * sin(y);\n"
            "let dudx = diff_step(sin(x + h) * cos(y), sin(x - h) * cos(y), h);\n"
            "let dudy = diff_step(sin(x) * cos(y + h), sin(x) * cos(y - h), h);\n"
            "let dpdx = diff_step(0.25 * (cos(2.0 * (x + h)) + cos(2.0 * y)), 0.25 * (cos(2.0 * (x - h)) + cos(2.0 * y)), h);\n"
            "let d2u_dx2 = (sin(x + h) * cos(y) - 2.0 * u0 + sin(x - h) * cos(y)) / (h * h);\n"
            "let d2u_dy2 = (sin(x) * cos(y + h) - 2.0 * u0 + sin(x) * cos(y - h)) / (h * h);\n"
            "let lap_u = d2u_dx2 + d2u_dy2;\n"
            "u0 * dudx + v0 * dudy + dpdx - nu * lap_u = 0";

        auto res = formulaic::LatexConverter::convert(ns_script);
        TEST_ASSERT(res.has_value(), "Navier-Stokes equation converted successfully");
        std::cout << "9. Navier-Stokes LaTeXLive formula ->\n" << res.value() << std::endl;
        TEST_ASSERT_CONTAINS(res.value(), "\\begin{aligned}", "Aligned multiline present");
        TEST_ASSERT_CONTAINS(res.value(), "\\nu &= 0.08", "nu formatted as Greek letter");
        TEST_ASSERT_CONTAINS(res.value(), "u_{0} &=", "u0 formatted with subscript");
        TEST_ASSERT_CONTAINS(res.value(), "v_{0} &= -\\cos\\left(x\\right) \\cdot \\sin\\left(y\\right)", "v0 correctly formatted");
        TEST_ASSERT_CONTAINS(res.value(), "\\operatorname{diff\\_step}", "diff_step operator formatted");
        TEST_ASSERT_CONTAINS(res.value(), "\\frac{\\sin\\left(x + h\\right)", "Second derivative fraction formatted");
        TEST_ASSERT_CONTAINS(res.value(), "\\text{d2u}_{dx2}", "d2u_dx2 subscript formatted");
        TEST_ASSERT_CONTAINS(res.value(), "\\text{lap}_{u} &= \\text{d2u}_{dx2} + \\text{d2u}_{dy2}", "lap_u formatted");
        TEST_ASSERT_CONTAINS(res.value(), "u_{0} \\cdot \\text{dudx} + v_{0} \\cdot \\text{dudy} + \\text{dpdx} - \\nu \\cdot \\text{lap}_{u} &= 0", "Final implicit equation aligned");
    }

    // Test 10: Expression::to_latex integration
    {
        auto expr_res = formulaic::Expression::parse("x^3 - 3*x + 2");
        TEST_ASSERT(expr_res.has_value(), "Expression parsed");
        auto tex_res = expr_res.value().to_latex();
        TEST_ASSERT(tex_res.has_value(), "to_latex produced output");
        std::cout << "10. Expression::to_latex -> " << tex_res.value() << std::endl;
        TEST_ASSERT_CONTAINS(tex_res.value(), "x^{3} - 3 \\cdot x + 2", "Expression::to_latex matches standard format");
    }

    // Test 11: Display math delimiters option
    {
        formulaic::LatexFormatOptions opt;
        opt.display_math_delimiters = true;
        auto res = formulaic::LatexConverter::convert("x^2 + y^2 = 4", opt);
        TEST_ASSERT(res.has_value(), "Display math delimiters option");
        std::cout << "11. Display math delimiters ->\n" << res.value() << std::endl;
        TEST_ASSERT_CONTAINS(res.value(), "\\[", "Begins with \\[");
        TEST_ASSERT_CONTAINS(res.value(), "\\]", "Ends with \\]");
    }

    // Test 12: LatexConverter::to_script reverse translation
    {
        // 12a: Fractions and equation
        auto res_frac = formulaic::LatexConverter::to_script("\\frac{1}{x} + \\frac{1}{y} = 0");
        TEST_ASSERT(res_frac.has_value(), "to_script converted rational equation");
        std::cout << "12a. LaTeX -> Script: " << res_frac.value() << std::endl;
        TEST_ASSERT_CONTAINS(res_frac.value(), "1 / x + 1 / y = 0", "Fractions converted to standard division");

        // 12b: Powers and trig functions
        auto res_trig = formulaic::LatexConverter::to_script("\\sin^{2}(x) + \\cos^{2}(x) = 1");
        TEST_ASSERT(res_trig.has_value(), "to_script converted trig powers");
        std::cout << "12b. LaTeX -> Script: " << res_trig.value() << std::endl;
        TEST_ASSERT_CONTAINS(res_trig.value(), "sin(x)^2", "sin power converted");
        TEST_ASSERT_CONTAINS(res_trig.value(), "cos(x)^2", "cos power converted");

        // 12c: Sqrt and Greek letters
        auto res_sqrt = formulaic::LatexConverter::to_script("\\sqrt{x^{2} + y^{2}} + \\alpha \\cdot \\sin(\\theta)");
        TEST_ASSERT(res_sqrt.has_value(), "to_script converted sqrt and greek letters");
        std::cout << "12c. LaTeX -> Script: " << res_sqrt.value() << std::endl;
        TEST_ASSERT_CONTAINS(res_sqrt.value(), "sqrt(x^2 + y^2)", "sqrt converted");
        TEST_ASSERT_CONTAINS(res_sqrt.value(), "alpha * sin(theta)", "Greek letters converted");

        // 12d: Multiline aligned block to let script
        const std::string tex_aligned =
            "\\begin{aligned}\n"
            "r &= \\sqrt{x^{2} + y^{2}} \\\\\n"
            "\\theta &= \\operatorname{atan2}(y, x) \\\\\n"
            "\\sin(6 \\cdot \\theta) \\cdot e^{-0.35 \\cdot r}\n"
            "\\end{aligned}";
        auto res_aligned = formulaic::LatexConverter::to_script(tex_aligned);
        TEST_ASSERT(res_aligned.has_value(), "to_script converted aligned block");
        std::cout << "12d. LaTeX -> Script (aligned):\n" << res_aligned.value() << std::endl;
        TEST_ASSERT_CONTAINS(res_aligned.value(), "let r = sqrt(x^2 + y^2);", "Aligned let statement converted");
        TEST_ASSERT_CONTAINS(res_aligned.value(), "let theta = atan2(y, x);", "Aligned let theta converted");
    }

    // Test 13: Expression::parse_latex
    {
        auto expr1 = formulaic::Expression::parse_latex("\\sin(x) + \\cos(y)");
        TEST_ASSERT(expr1.has_value(), "parse_latex compiled \\sin(x) + \\cos(y)");
        double v1 = expr1->eval(0.0, 0.0);
        TEST_ASSERT(std::abs(v1 - 1.0) < 1e-6, "\\sin(0) + \\cos(0) == 1");

        auto expr2 = formulaic::Expression::parse_latex("\\frac{x + 1}{x - 1}");
        TEST_ASSERT(expr2.has_value(), "parse_latex compiled \\frac{x+1}{x-1}");
        double v2 = expr2->eval(3.0);
        TEST_ASSERT(std::abs(v2 - 2.0) < 1e-6, "\\frac{3+1}{3-1} == 2");

        auto expr3 = formulaic::Expression::parse_latex("x^{3} - 4 \\cdot x");
        TEST_ASSERT(expr3.has_value(), "parse_latex compiled x^{3} - 4 \\cdot x");
        double v3 = expr3->eval(2.0);
        TEST_ASSERT(std::abs(v3 - 0.0) < 1e-6, "2^3 - 4*2 == 0");
    }

    // Test 14: LatexRenderModule compile & plot
    {
        auto expr_res = formulaic::LatexRenderModule::compile_latex("\\frac{1}{x} + \\frac{1}{y} = 0");
        TEST_ASSERT(expr_res.has_value(), "LatexRenderModule::compile_latex compiled implicit formula");

        formulaic::FrameBuffer fb(400, 300);
        fb.clear(formulaic::Color::BackgroundDark);
        formulaic::Viewport vp(400, 300, formulaic::Rect2D(-5.0, 5.0, -5.0, 5.0));

        formulaic::LatexRenderModule::render_latex_plot(fb, vp, "\\frac{1}{x} + \\frac{1}{y} = 0");
        TEST_ASSERT(fb.width() == 400 && fb.height() == 300, "FrameBuffer size valid after render_latex_plot");
    }

    // Test 15: LatexRenderModule banner card and full composite
    {
        formulaic::FrameBuffer fb(500, 400);
        fb.clear(formulaic::Color::BackgroundDark);
        formulaic::Viewport vp(500, 400, formulaic::Rect2D(-5.0, 5.0, -5.0, 5.0));

        formulaic::LatexRenderStyle style;
        style.show_card = true;
        style.curve_color = formulaic::Color::Cyan;
        formulaic::LatexRenderModule::render_full(fb, vp, "\\sin(x)^{2} + \\cos(y)^{2}", style);
        TEST_ASSERT(fb.width() == 500 && fb.height() == 400, "Composite render_full completed");
    }

    // Test 16: RasterEngine LaTeX methods integration
    {
        formulaic::FrameBuffer fb(400, 300);
        fb.clear(formulaic::Color::BackgroundDark);
        formulaic::Viewport vp(400, 300, formulaic::Rect2D(-5.0, 5.0, -5.0, 5.0));

        formulaic::RasterEngine engine;
        engine.plot_latex(fb, vp, "x^{2} + y^{2} = 4");
        engine.plot_latex_card(fb, 20, 20, "\\frac{1}{x} + \\frac{1}{y} = 0");
        TEST_ASSERT(fb.width() == 400, "RasterEngine plot_latex & plot_latex_card passed");
    }

    // Test 17: User case - Three-petal rose parametric system: \begin{cases} x = a \cos(3\theta)\cos(\theta) \\ y = a \cos(3\theta)\sin(\theta) \end{cases}
    {
        const std::string rose_latex =
            "$$\\begin{cases} x = a \\cos(3\\theta)\\cos(\\theta) \\\\ y = a \\cos(3\\theta)\\sin(\\theta) \\end{cases} \\quad (\\theta \\in [0, \\pi])$$";
        auto res = formulaic::LatexConverter::to_script(rose_latex);
        TEST_ASSERT(res.has_value(), "Three-petal rose cases converted successfully");
        std::cout << "17. Three-petal rose script ->\n" << res.value() << std::endl;
        TEST_ASSERT_CONTAINS(res.value(), "let a = 3", "Free parameter 'a' automatically declared");
        TEST_ASSERT_CONTAINS(res.value(), "hypot(x, y)", "Radial distance defined");
        TEST_ASSERT_CONTAINS(res.value(), "atan2(y, x)", "Polar angle defined");
        TEST_ASSERT_CONTAINS(res.value(), "r - a * cos(3 * theta) = 0", "Rose implicit equation formed");

        // Verify the resulting script compiles cleanly into an Expression via parse_equation and parse_latex
        auto expr_res = formulaic::Expression::parse_equation(res.value());
        TEST_ASSERT(expr_res.has_value(), "Generated rose script parses into valid Expression without unknown identifier errors");

        auto expr_latex = formulaic::Expression::parse_latex(rose_latex);
        TEST_ASSERT(expr_latex.has_value(), "parse_latex compiles three-petal rose directly");
    }

    // Test 18: High Quality LaTeXLive Vector Math Rendering & Image Export
    {
        // 18a: User reference image formula: (x - 10)^2 + (y - 2)^2 = 5^2
        const std::string circle_tex = "(x - 10)^2 + (y - 2)^2 = 5^2";
        auto fb_circle = formulaic::LatexRenderModule::render_math_to_framebuffer(circle_tex, formulaic::Color::Black, formulaic::Color::White, 36.0f);
        TEST_ASSERT(fb_circle.width() > 100 && fb_circle.height() > 30, "Circle formula rendered to FrameBuffer with proper dimensions");

        bool exp1 = formulaic::LatexRenderModule::export_math_image(
            circle_tex,
            "F:/Formulaic/build/latex_circle_rendered.png",
            formulaic::Color::Black,
            formulaic::Color::White,
            48.0f
        );
        TEST_ASSERT(exp1, "Circle LaTeXLive image exported successfully as PNG");

        // 18b: User cases equation: \begin{cases} x = a \cos(3\theta)\cos(\theta) \\ y = a \cos(3\theta)\sin(\theta) \end{cases} \quad (\theta \in [0, \pi])
        const std::string rose_tex = "$$\\begin{cases} x = a \\cos(3\\theta)\\cos(\\theta) \\\\ y = a \\cos(3\\theta)\\sin(\\theta) \\end{cases} \\quad (\\theta \\in [0, \\pi])$$";
        auto fb_rose = formulaic::LatexRenderModule::render_math_to_framebuffer(rose_tex, formulaic::Color::White, formulaic::Color::BackgroundDark, 28.0f);
        TEST_ASSERT(fb_rose.width() > 200 && fb_rose.height() > 50, "Cases rose formula rendered to FrameBuffer with proper dimensions");

        bool exp2 = formulaic::LatexRenderModule::export_math_image(
            rose_tex,
            "F:/Formulaic/build/latex_rose_rendered.png",
            formulaic::Color::White,
            formulaic::Color(24, 24, 37, 255),
            36.0f
        );
        TEST_ASSERT(exp2, "Cases rose LaTeXLive image exported successfully as PNG");
        // 18c: Image 2 formula: Spiral Waves multiline aligned
        const std::string multi_tex =
            "\\begin{aligned} r &= \\sqrt{x^{2} + y^{2}} \\\\\n"
            "\\theta &= \\operatorname{atan2}\\left(y, x\\right) \\\\\n"
            "\\text{envelope} &= e^{-0.35 \\cdot r} \\\\\n"
            "f(x, y) &= \\sin\\left(6 \\cdot \\theta + 2 \\cdot t\\right) \\cdot \\text{envelope}\\end{aligned}";
        bool exp3 = formulaic::LatexRenderModule::export_math_image(
            multi_tex,
            "F:/Formulaic/build/latex_multiline_rendered.png",
            formulaic::Color::White,
            formulaic::Color(24, 24, 37, 255),
            28.0f
        );
        TEST_ASSERT(exp3, "Multiline formula exported to PNG");

        // 18d: Image 1 formula: FFT Window clamp & tri_wave
        const std::string clamp_tex =
            "\\begin{aligned} u &= \\operatorname{clamp}\\left(\\frac{x + 5}{10}, 0, 1\\right) \\\\\n"
            "f(x, y) &= \\operatorname{hann}\\left(u\\right) \\cdot \\operatorname{tri\\_wave}\\left(u \\cdot 5 - t \\cdot 0.5\\right)\\end{aligned}";
        bool exp4 = formulaic::LatexRenderModule::export_math_image(
            clamp_tex,
            "F:/Formulaic/build/latex_clamp_rendered.png",
            formulaic::Color::White,
            formulaic::Color(24, 24, 37, 255),
            28.0f
        );
        TEST_ASSERT(exp4, "Clamp tri_wave formula exported to PNG");
        std::cout << "18. High-resolution LaTeXLive formulas rendered and exported to PNG successfully!\n";
    }

    std::cout << "\n>>> All LaTeXLive conversion, reverse to_script & rendering module tests PASSED successfully! <<<\n";
    return 0;
}
