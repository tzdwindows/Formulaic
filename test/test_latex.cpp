#include <Formulaic/parser/expression.hpp>
#include <Formulaic/parser/latex_converter.hpp>
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

    std::cout << "\n>>> All LaTeXLive conversion tests PASSED successfully! <<<\n";
    return 0;
}
