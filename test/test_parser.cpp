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

    std::cout << "\n>>> All Parser & Evaluation Engine Tests PASSED successfully! <<<\n";
    return 0;
}
