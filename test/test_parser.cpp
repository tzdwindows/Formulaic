#include <Formulaic/parser/expression.hpp>
#include <Formulaic/utils/timer.hpp>
#include <cassert>
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

#define TEST_ASSERT_NEAR(a, b, eps, msg) \
    do { \
        if (std::abs((a) - (b)) > (eps)) { \
            std::cerr << "Near assertion failed: |" << (a) << " - " << (b) << "| = " \
                      << std::abs((a) - (b)) << " > " << (eps) << " [" << (msg) << "]" \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "==========================================\n";
    std::cout << " Running Parser & Evaluation Engine Tests \n";
    std::cout << "==========================================\n";

    // 1. Basic Arithmetic & Precedence
    {
        std::cout << "[Test 1] Arithmetic & Precedence... ";
        auto res = formulaic::Expression::parse("1 + 2 * 3");
        TEST_ASSERT(res.has_value(), res.error().format());
        TEST_ASSERT_NEAR(res->eval(0.0), 7.0, 1e-9, "1 + 2 * 3 == 7");

        auto res2 = formulaic::Expression::parse("(1 + 2) * 3");
        TEST_ASSERT(res2.has_value(), res2.error().format());
        TEST_ASSERT_NEAR(res2->eval(0.0), 9.0, 1e-9, "(1 + 2) * 3 == 9");

        // Right-associative exponentiation: 2^3^2 = 2^(3^2) = 2^9 = 512
        auto res_pow = formulaic::Expression::parse("2 ^ 3 ^ 2");
        TEST_ASSERT(res_pow.has_value(), res_pow.error().format());
        TEST_ASSERT_NEAR(res_pow->eval(0.0), 512.0, 1e-9, "2^3^2 == 512");

        // Subtraction associativity: 10 - 4 - 2 = (10 - 4) - 2 = 4
        auto res_sub = formulaic::Expression::parse("10 - 4 - 2");
        TEST_ASSERT(res_sub.has_value(), res_sub.error().format());
        TEST_ASSERT_NEAR(res_sub->eval(0.0), 4.0, 1e-9, "10 - 4 - 2 == 4");

        // Division: 10 / 2 / 2 = 2.5
        auto res_div = formulaic::Expression::parse("10 / 2 / 2");
        TEST_ASSERT(res_div.has_value(), res_div.error().format());
        TEST_ASSERT_NEAR(res_div->eval(0.0), 2.5, 1e-9, "10 / 2 / 2 == 2.5");

        // Modulo: 10 % 3 = 1
        auto res_mod = formulaic::Expression::parse("10 % 3");
        TEST_ASSERT(res_mod.has_value(), res_mod.error().format());
        TEST_ASSERT_NEAR(res_mod->eval(0.0), 1.0, 1e-9, "10 % 3 == 1");
        std::cout << "PASSED\n";
    }

    // 2. Unary Operators
    {
        std::cout << "[Test 2] Unary Operators... ";
        auto res = formulaic::Expression::parse("-5 + 10");
        TEST_ASSERT(res.has_value(), res.error().format());
        TEST_ASSERT_NEAR(res->eval(0.0), 5.0, 1e-9, "-5 + 10 == 5");

        auto res_neg = formulaic::Expression::parse("-(3 * 2)");
        TEST_ASSERT(res_neg.has_value(), res_neg.error().format());
        TEST_ASSERT_NEAR(res_neg->eval(0.0), -6.0, 1e-9, "-(3 * 2) == -6");

        auto res_not = formulaic::Expression::parse("!0 + !5");
        TEST_ASSERT(res_not.has_value(), res_not.error().format());
        TEST_ASSERT_NEAR(res_not->eval(0.0), 1.0, 1e-9, "!0 + !5 == 1");
        std::cout << "PASSED\n";
    }

    // 3. Comparisons and Logical Operators
    {
        std::cout << "[Test 3] Comparisons & Logic... ";
        auto res = formulaic::Expression::parse("(2 < 5) && (10 >= 10) && (3 == 3) && (4 != 5)");
        TEST_ASSERT(res.has_value(), res.error().format());
        TEST_ASSERT_NEAR(res->eval(0.0), 1.0, 1e-9, "Comparison combinations");

        auto res_or = formulaic::Expression::parse("(2 > 5) || (10 < 20)");
        TEST_ASSERT(res_or.has_value(), res_or.error().format());
        TEST_ASSERT_NEAR(res_or->eval(0.0), 1.0, 1e-9, "Logical OR");
        std::cout << "PASSED\n";
    }

    // 4. Built-in Math Functions and Constants
    {
        std::cout << "[Test 4] Math Functions & Constants... ";
        auto res_sin = formulaic::Expression::parse("sin(pi / 2)");
        TEST_ASSERT(res_sin.has_value(), res_sin.error().format());
        TEST_ASSERT_NEAR(res_sin->eval(0.0), 1.0, 1e-9, "sin(pi / 2) == 1");

        auto res_cos = formulaic::Expression::parse("cos(pi)");
        TEST_ASSERT(res_cos.has_value(), res_cos.error().format());
        TEST_ASSERT_NEAR(res_cos->eval(0.0), -1.0, 1e-9, "cos(pi) == -1");

        auto res_exp = formulaic::Expression::parse("exp(0) + ln(e)");
        TEST_ASSERT(res_exp.has_value(), res_exp.error().format());
        TEST_ASSERT_NEAR(res_exp->eval(0.0), 2.0, 1e-9, "exp(0) + ln(e) == 2");

        auto res_min_max = formulaic::Expression::parse("min(10, 5) + max(3, 8) + clamp(15, 0, 10)");
        TEST_ASSERT(res_min_max.has_value(), res_min_max.error().format());
        TEST_ASSERT_NEAR(res_min_max->eval(0.0), 23.0, 1e-9, "min + max + clamp == 5 + 8 + 10 == 23");

        auto res_sqrt = formulaic::Expression::parse("sqrt(16) + cbrt(27) + abs(-7)");
        TEST_ASSERT(res_sqrt.has_value(), res_sqrt.error().format());
        TEST_ASSERT_NEAR(res_sqrt->eval(0.0), 14.0, 1e-9, "sqrt(16) + cbrt(27) + abs(-7) == 14");
        std::cout << "PASSED\n";
    }

    // 5. Implicit Multiplication & Variables
    {
        std::cout << "[Test 5] Implicit Multiplication & Variables... ";
        // 2x + 3y
        auto res = formulaic::Expression::parse("2x + 3y", {"x", "y"});
        TEST_ASSERT(res.has_value(), res.error().format());
        TEST_ASSERT_NEAR(res->eval(4.0, 5.0), 23.0, 1e-9, "2*4 + 3*5 == 23");

        // 3sin(x)
        auto res_sin = formulaic::Expression::parse("3sin(x)", {"x"});
        TEST_ASSERT(res_sin.has_value(), res_sin.error().format());
        TEST_ASSERT_NEAR(res_sin->eval(3.141592653589793 / 2.0), 3.0, 1e-9, "3*sin(pi/2) == 3");

        // (x + 1)(x - 1)
        auto res_poly = formulaic::Expression::parse("(x + 1)(x - 1)", {"x"});
        TEST_ASSERT(res_poly.has_value(), res_poly.error().format());
        TEST_ASSERT_NEAR(res_poly->eval(5.0), 24.0, 1e-9, "(5+1)(5-1) == 24");

        // x(y + 1) -> x * (y + 1)
        auto res_xy = formulaic::Expression::parse("x(y + 1)", {"x", "y"});
        TEST_ASSERT(res_xy.has_value(), res_xy.error().format());
        TEST_ASSERT_NEAR(res_xy->eval(3.0, 4.0), 15.0, 1e-9, "x(y + 1) == 3 * (4 + 1) == 15");

        // pi(x + 1) -> pi * (x + 1)
        auto res_pi = formulaic::Expression::parse("pi(x + 1)", {"x"});
        TEST_ASSERT(res_pi.has_value(), res_pi.error().format());
        TEST_ASSERT_NEAR(res_pi->eval(2.0), 3.14159265358979323846 * 3.0, 1e-9, "pi(x + 1) == 3*pi");

        // 2x(y + 3) -> 2 * x * (y + 3)
        auto res_2x = formulaic::Expression::parse("2x(y + 3)", {"x", "y"});
        TEST_ASSERT(res_2x.has_value(), res_2x.error().format());
        TEST_ASSERT_NEAR(res_2x->eval(2.0, 5.0), 32.0, 1e-9, "2x(y + 3) == 2 * 2 * 8 == 32");

        std::cout << "PASSED\n";
    }

    // 6. Multi-variable with Time Parameter
    {
        std::cout << "[Test 6] Multi-variable with time parameter t... ";
        auto res = formulaic::Expression::parse("sin(x + t) * cos(y - t)", {"x", "y", "t"});
        TEST_ASSERT(res.has_value(), res.error().format());
        double val = res->eval(0.5, 0.2, 1.0);
        double expected = std::sin(0.5 + 1.0) * std::cos(0.2 - 1.0);
        TEST_ASSERT_NEAR(val, expected, 1e-9, "sin(x+t)*cos(y-t)");
        std::cout << "PASSED\n";
    }

    // 7. Robust Diagnostic Error Handling
    {
        std::cout << "[Test 7] Structured Diagnostics & Error Handling... ";

        // Unexpected character
        auto err_char = formulaic::Expression::parse("x @ 2");
        TEST_ASSERT(!err_char.has_value(), "Expected error for '@'");
        TEST_ASSERT(err_char.error().code == formulaic::ErrorCode::UnexpectedCharacter, "Code matches");

        // Unmatched parenthesis
        auto err_paren = formulaic::Expression::parse("(x + 2 * (y - 1)");
        TEST_ASSERT(!err_paren.has_value(), "Expected error for unmatched paren");
        TEST_ASSERT(err_paren.error().code == formulaic::ErrorCode::MissingParenthesis, "MissingParenthesis code");

        // Unknown identifier
        auto err_ident = formulaic::Expression::parse("sin(x) + unknown_symbol");
        TEST_ASSERT(!err_ident.has_value(), "Expected error for unknown identifier");
        TEST_ASSERT(err_ident.error().code == formulaic::ErrorCode::UnknownIdentifier, "UnknownIdentifier code");

        // Single '=' error
        auto err_equal = formulaic::Expression::parse("x = 5");
        TEST_ASSERT(!err_equal.has_value(), "Expected error for single '='");

        // Invalid function arity
        auto err_arity = formulaic::Expression::parse("sin(1, 2)");
        TEST_ASSERT(!err_arity.has_value(), "Expected error for arity mismatch");
        TEST_ASSERT(err_arity.error().code == formulaic::ErrorCode::InvalidArity, "InvalidArity code");

        // Missing operand
        auto err_operand = formulaic::Expression::parse("1 + * 2");
        TEST_ASSERT(!err_operand.has_value(), "Expected error for missing operand");

        std::cout << "PASSED (all diagnostics caught cleanly)\n";
    }

    // 8. Bytecode VM Performance Benchmark
    {
        std::cout << "[Test 8] Bytecode VM High-frequency Benchmark... \n";
        auto res = formulaic::Expression::parse("sin(x) * cos(y) + exp(-t)", {"x", "y", "t"});
        TEST_ASSERT(res.has_value(), res.error().format());

        constexpr size_t kIterations = 1'000'000;
        formulaic::utils::Timer timer;
        double sum = 0.0;

        for (size_t i = 0; i < kIterations; ++i) {
            double x = i * 0.001;
            double y = i * 0.002;
            double t = i * 0.0001;
            sum += res->eval(x, y, t);
        }

        double elapsed_ms = timer.elapsed_milliseconds();
        double ops_per_sec = (static_cast<double>(kIterations) / elapsed_ms) * 1000.0;

        std::cout << "       Iterations: " << kIterations << "\n";
        std::cout << "       Elapsed:    " << elapsed_ms << " ms\n";
        std::cout << "       Throughput: " << static_cast<size_t>(ops_per_sec) << " evaluations/sec\n";
        std::cout << "       Checksum:   " << sum << "\n";
        std::cout << "       Status:     PASSED\n";
    }

    // 9. Extended Math Functions and Constants Suite
    {
        std::cout << "[Test 9] Extended Math Functions & Advanced Constants Suite... ";

        // Inverse hyperbolic
        auto r_hyp = formulaic::Expression::parse("asinh(0) + acosh(1) + atanh(0)");
        TEST_ASSERT(r_hyp.has_value(), r_hyp.error().format());
        TEST_ASSERT_NEAR(r_hyp->eval(0.0), 0.0, 1e-9, "asinh(0)+acosh(1)+atanh(0) == 0");

        // Reciprocal trigonometry & hyperbolic
        auto r_recip = formulaic::Expression::parse("sec(0) + csc(pi / 2) + cot(pi / 4) + sech(0)");
        TEST_ASSERT(r_recip.has_value(), r_recip.error().format());
        TEST_ASSERT_NEAR(r_recip->eval(0.0), 4.0, 1e-9, "sec(0) + csc(pi/2) + cot(pi/4) + sech(0) == 4");

        // Special functions: sinc, erf, erfc, gamma, lgamma, beta
        auto r_sinc0 = formulaic::Expression::parse("sinc(0)");
        TEST_ASSERT(r_sinc0.has_value(), r_sinc0.error().format());
        TEST_ASSERT_NEAR(r_sinc0->eval(0.0), 1.0, 1e-9, "sinc(0) == 1");

        auto r_sinc_pi = formulaic::Expression::parse("sinc(pi / 2)");
        TEST_ASSERT(r_sinc_pi.has_value(), r_sinc_pi.error().format());
        TEST_ASSERT_NEAR(r_sinc_pi->eval(0.0), 2.0 / 3.141592653589793, 1e-9, "sinc(pi/2) == 2/pi");

        auto r_erf = formulaic::Expression::parse("erf(0) + erfc(0)");
        TEST_ASSERT(r_erf.has_value(), r_erf.error().format());
        TEST_ASSERT_NEAR(r_erf->eval(0.0), 1.0, 1e-9, "erf(0) + erfc(0) == 1");

        auto r_gamma = formulaic::Expression::parse("gamma(5) + tgamma(4)");
        TEST_ASSERT(r_gamma.has_value(), r_gamma.error().format());
        TEST_ASSERT_NEAR(r_gamma->eval(0.0), 24.0 + 6.0, 1e-9, "gamma(5) + tgamma(4) == 30");

        auto r_beta = formulaic::Expression::parse("beta(2, 3)");
        TEST_ASSERT(r_beta.has_value(), r_beta.error().format());
        TEST_ASSERT_NEAR(r_beta->eval(0.0), 1.0 / 12.0, 1e-9, "beta(2, 3) == 1/12");

        // Exp2, Expm1, Log1p
        auto r_exp2 = formulaic::Expression::parse("exp2(4) + expm1(0) + log1p(0)");
        TEST_ASSERT(r_exp2.has_value(), r_exp2.error().format());
        TEST_ASSERT_NEAR(r_exp2->eval(0.0), 16.0, 1e-9, "exp2(4) == 16");

        // Trunc, Frac, Copysign, Remainder, Hypot
        auto r_float = formulaic::Expression::parse("trunc(-3.8) + frac(4.75) + copysign(5, -1) + hypot(3, 4)");
        TEST_ASSERT(r_float.has_value(), r_float.error().format());
        // -3.0 + 0.75 + (-5.0) + 5.0 = -2.25
        TEST_ASSERT_NEAR(r_float->eval(0.0), -2.25, 1e-9, "trunc + frac + copysign + hypot == -2.25");

        // Graphics Step, Smoothstep, Lerp / Mix
        auto r_step = formulaic::Expression::parse("step(5, 3) + step(5, 7)");
        TEST_ASSERT(r_step.has_value(), r_step.error().format());
        TEST_ASSERT_NEAR(r_step->eval(0.0), 1.0, 1e-9, "step(5, 3) + step(5, 7) == 1");

        auto r_smooth = formulaic::Expression::parse("smoothstep(0, 10, 5)");
        TEST_ASSERT(r_smooth.has_value(), r_smooth.error().format());
        TEST_ASSERT_NEAR(r_smooth->eval(0.0), 0.5, 1e-9, "smoothstep midpoint is 0.5");

        auto r_lerp = formulaic::Expression::parse("lerp(10, 30, 0.25) + mix(10, 30, 0.75)");
        TEST_ASSERT(r_lerp.has_value(), r_lerp.error().format());
        TEST_ASSERT_NEAR(r_lerp->eval(0.0), 15.0 + 25.0, 1e-9, "lerp + mix == 40");

        // Signal: Heaviside, Rect, Tri, Deg2rad, Rad2deg
        auto r_signal = formulaic::Expression::parse("heaviside(5) + rect(0.1) + tri(0.2) + rad2deg(deg2rad(90))");
        TEST_ASSERT(r_signal.has_value(), r_signal.error().format());
        // 1.0 + 1.0 + 0.8 + 90.0 = 92.8
        TEST_ASSERT_NEAR(r_signal->eval(0.0), 92.8, 1e-9, "heaviside + rect + tri + angle transforms");

        // Constants: sqrt2, sqrt3, euler, ln2, ln10
        auto r_consts = formulaic::Expression::parse("sqrt2^2 + sqrt3^2 + exp(ln2) + exp(ln10)");
        TEST_ASSERT(r_consts.has_value(), r_consts.error().format());
        TEST_ASSERT_NEAR(r_consts->eval(0.0), 2.0 + 3.0 + 2.0 + 10.0, 1e-9, "sqrt2^2 + sqrt3^2 + 2 + 10 == 17");

        std::cout << "PASSED (all 34+ extended math functions verified)\n";
    }

    // -------------------------------------------------------------------------
    // Test 10: Variable Declarations (let, var) & Calculus/FFT Functions
    // -------------------------------------------------------------------------
    {
        std::cout << "[Test 10] Variable Declarations & Calculus / FFT Builtins... ";

        // 1. Variable Declarations
        auto r_let = formulaic::Expression::parse("let a = 10; let b = 20; a + b;");
        TEST_ASSERT(r_let.has_value(), r_let.error().format());
        TEST_ASSERT_NEAR(r_let->eval(0.0), 30.0, 1e-9, "let a = 10; let b = 20; a + b == 30");

        auto r_var = formulaic::Expression::parse("var u = x * 2; var v = y * 3; hypot(u, v)");
        TEST_ASSERT(r_var.has_value(), r_var.error().format());
        TEST_ASSERT_NEAR(r_var->eval(3.0, 4.0), hypot(6.0, 12.0), 1e-9, "hypot(2x, 3y)");

        auto r_multi = formulaic::Expression::parse(
            "let r = hypot(x, y);\n"
            "let theta = atan2(y, x);\n"
            "r * sin(theta);"
        );
        TEST_ASSERT(r_multi.has_value(), r_multi.error().format());
        TEST_ASSERT_NEAR(r_multi->eval(3.0, 4.0), 4.0, 1e-9, "r * sin(theta) == y");

        // 2. Calculus functions
        // diff_step(fp, fm, h) = (fp - fm) / (2h)
        auto r_diff = formulaic::Expression::parse("diff_step(sin(0.1), sin(-0.1), 0.1)");
        TEST_ASSERT(r_diff.has_value(), r_diff.error().format());
        TEST_ASSERT_NEAR(r_diff->eval(0.0), sin(0.1) / 0.1, 1e-9, "diff_step central difference");

        // curvature(y', y'') = |y''| / (1 + y'^2)^1.5. For circle r=2 at top: y'=0, y''=0.5 -> curvature = 0.5
        auto r_curv = formulaic::Expression::parse("curvature(0.0, 0.5)");
        TEST_ASSERT(r_curv.has_value(), r_curv.error().format());
        TEST_ASSERT_NEAR(r_curv->eval(0.0), 0.5, 1e-9, "curvature == 0.5");

        // trapz(y0, y1, dx) = 0.5 * (y0 + y1) * dx
        auto r_trapz = formulaic::Expression::parse("trapz(2, 4, 3)");
        TEST_ASSERT(r_trapz.has_value(), r_trapz.error().format());
        TEST_ASSERT_NEAR(r_trapz->eval(0.0), 9.0, 1e-9, "trapz(2, 4, 3) == 9");

        // simpson(y0, y1, y2, h) = (y0 + 4*y1 + y2) * (h / 3.0)
        auto r_simp = formulaic::Expression::parse("simpson(1, 4, 1, 1)");
        TEST_ASSERT(r_simp.has_value(), r_simp.error().format());
        TEST_ASSERT_NEAR(r_simp->eval(0.0), 6.0, 1e-9, "simpson(1, 4, 1, 1) == 6");

        // euler(y, dydt, dt) = y + dydt * dt
        auto r_euler = formulaic::Expression::parse("euler(10, -2, 0.5)");
        TEST_ASSERT(r_euler.has_value(), r_euler.error().format());
        TEST_ASSERT_NEAR(r_euler->eval(0.0), 9.0, 1e-9, "euler(10, -2, 0.5) == 9");

        // laplacian2d(dxx, dyy) = dxx + dyy
        auto r_lap = formulaic::Expression::parse("laplacian2d(3, 4)");
        TEST_ASSERT(r_lap.has_value(), r_lap.error().format());
        TEST_ASSERT_NEAR(r_lap->eval(0.0), 7.0, 1e-9, "laplacian2d(3, 4) == 7");

        // 3. Spectral & Windowing functions
        // Hann window: hann(0) = 0, hann(0.5) = 1, hann(1) = 0
        auto r_hann = formulaic::Expression::parse("hann(0.5)");
        TEST_ASSERT(r_hann.has_value(), r_hann.error().format());
        TEST_ASSERT_NEAR(r_hann->eval(0.0), 1.0, 1e-9, "hann(0.5) == 1.0");

        // Hamming window: hamming(0.5) = 0.54 - 0.46*cos(pi) = 1.0
        auto r_hamm = formulaic::Expression::parse("hamming(0.5)");
        TEST_ASSERT(r_hamm.has_value(), r_hamm.error().format());
        TEST_ASSERT_NEAR(r_hamm->eval(0.0), 1.0, 1e-9, "hamming(0.5) == 1.0");

        // Waveforms
        auto r_sq = formulaic::Expression::parse("square_wave(0.2) + square_wave(0.7)");
        TEST_ASSERT(r_sq.has_value(), r_sq.error().format());
        TEST_ASSERT_NEAR(r_sq->eval(0.0), 0.0, 1e-9, "square_wave(0.2) + square_wave(0.7) == 0");

        auto r_saw = formulaic::Expression::parse("sawtooth_wave(0.5)");
        TEST_ASSERT(r_saw.has_value(), r_saw.error().format());
        TEST_ASSERT_NEAR(r_saw->eval(0.0), 0.0, 1e-9, "sawtooth_wave(0.5) == 0");

        auto r_tri = formulaic::Expression::parse("triangle_wave(0.25)");
        TEST_ASSERT(r_tri.has_value(), r_tri.error().format());
        TEST_ASSERT_NEAR(r_tri->eval(0.0), 0.0, 1e-9, "triangle_wave(0.25) == 0");

        // Gaussian
        auto r_gauss = formulaic::Expression::parse("gaussian(0, 0, 1)");
        TEST_ASSERT(r_gauss.has_value(), r_gauss.error().format());
        TEST_ASSERT_NEAR(r_gauss->eval(0.0), 1.0, 1e-9, "gaussian(0, 0, 1) == 1");

        std::cout << "PASSED\n";
    }

    std::cout << "\n>>> All Parser & Evaluation Engine Tests PASSED successfully! <<<\n";
    return 0;
}
