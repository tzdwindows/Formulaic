#include <Formulaic/math/bigint.hpp>
#include <Formulaic/math/rational.hpp>
#include <Formulaic/math/gmp_evaluator.hpp>
#include <Formulaic/parser/expression.hpp>
#include <cassert>
#include <cmath>
#include <iostream>

using namespace formulaic::math;

static void test_bigint_basics() {
    std::cout << "[Test] BigInt basics...\n";
    assert(has_gmp_support());
    std::cout << "GMP Backend: " << get_gmp_backend_info() << "\n";

    BigInt a(123456789012345LL);
    BigInt b(987654321098765LL);
    BigInt c = a + b;
    assert(c == BigInt("1111111110111110"));

    BigInt mul = a * b;
    assert(mul.to_string() == "121932631137021160352528148725");

    BigInt q = mul / a;
    BigInt r = mul % a;
    assert(q == b);
    assert(r.is_zero());

    // Power
    BigInt p2 = BigInt::pow(BigInt(2), 64);
    assert(p2 == BigInt("18446744073709551616"));
    BigInt p2_minus_1 = p2 - 1;
    assert(p2_minus_1 == BigInt("18446744073709551615"));

    // Bitwise
    BigInt bit_a(0b1100);
    BigInt bit_b(0b1010);
    assert((bit_a & bit_b) == BigInt(0b1000));
    assert((bit_a | bit_b) == BigInt(0b1110));
    assert((bit_a ^ bit_b) == BigInt(0b0110));
    assert((bit_a << 2) == BigInt(0b110000));
    assert((bit_a >> 1) == BigInt(0b0110));
    std::cout << "  Passed BigInt basics!\n";
}

static void test_number_theory() {
    std::cout << "[Test] BigInt number theory functions...\n";

    // GCD & LCM
    assert(BigInt::gcd(BigInt(1071), BigInt(462)) == BigInt(21));
    assert(BigInt::gcd(BigInt(0), BigInt(42)) == BigInt(42));
    assert(BigInt::lcm(BigInt(24), BigInt(36)) == BigInt(72));

    // Factorial
    assert(BigInt::factorial(0) == BigInt(1));
    assert(BigInt::factorial(5) == BigInt(120));
    assert(BigInt::factorial(10) == BigInt(3628800));
    BigInt f30 = BigInt::factorial(30);
    assert(f30 == BigInt("265252859812191058636308480000000"));

    // Binomial
    assert(BigInt::binomial(10, 3) == BigInt(120));
    assert(BigInt::binomial(50, 6) == BigInt(15890700));

    // Fibonacci
    assert(BigInt::fibonacci(0) == BigInt(0));
    assert(BigInt::fibonacci(1) == BigInt(1));
    assert(BigInt::fibonacci(2) == BigInt(1));
    assert(BigInt::fibonacci(10) == BigInt(55));
    assert(BigInt::fibonacci(50) == BigInt("12586269025"));

    // Power Modulo & Square Root
    assert(BigInt::pow_mod(BigInt(2), BigInt(10), BigInt(1000)) == BigInt(24));
    assert(BigInt::sqrt(BigInt(144)) == BigInt(12));
    assert(BigInt::sqrt(BigInt("10000000000000000000000000000000000000000")) == BigInt("100000000000000000000"));

    // Primes
    assert(BigInt(17).is_probab_prime());
    assert(BigInt(31).is_probab_prime());
    assert(!BigInt(18).is_probab_prime());
    assert(BigInt(14).next_prime() == BigInt(17));
    assert(BigInt(31).next_prime() == BigInt(37));
    std::cout << "  Passed BigInt number theory!\n";
}

static void test_rational_basics() {
    std::cout << "[Test] Rational basics...\n";

    Rational r1(1, 3);
    Rational r2(1, 6);
    Rational sum = r1 + r2;
    assert(sum == Rational(1, 2));
    assert(sum.to_string() == "1/2");

    Rational diff = r1 - r2;
    assert(diff == Rational(1, 6));

    Rational prod = Rational(3, 4) * Rational(8, 9);
    assert(prod == Rational(2, 3));

    Rational quot = Rational(2, 3) / Rational(4, 9);
    assert(quot == Rational(3, 2));

    // Canonicalization
    Rational unreduced("12/16");
    assert(unreduced == Rational(3, 4));
    assert(unreduced.num() == BigInt(3));
    assert(unreduced.den() == BigInt(4));

    // Decimal float conversion
    assert(std::abs(Rational(1, 4).to_double() - 0.25) < 1e-12);
    assert(Rational(3, 2).inv() == Rational(2, 3));
    std::cout << "  Passed Rational basics!\n";
}

static void test_gmp_evaluator() {
    std::cout << "[Test] GmpEvaluator exact evaluation...\n";

    auto res_int1 = GmpEvaluator::eval_int("100! / (98! * 2!)");
    assert(res_int1.has_value());
    assert(res_int1.value() == BigInt(4950));

    auto res_gcd = GmpEvaluator::eval_int("gcd(123456, 7890)");
    assert(res_gcd.has_value());
    assert(res_gcd.value() == BigInt(6));

    auto res_bin = GmpEvaluator::eval_int("binomial(12, 4)");
    assert(res_bin.has_value());
    assert(res_bin.value() == BigInt(495));

    auto res_fib = GmpEvaluator::eval_int("fib(20)");
    assert(res_fib.has_value());
    assert(res_fib.value() == BigInt(6765));

    auto res_pow = GmpEvaluator::eval_int("2^64 - 1");
    assert(res_pow.has_value());
    assert(res_pow.value() == BigInt("18446744073709551615"));

    auto res_rat = GmpEvaluator::eval_rational("1/3 + 1/6");
    assert(res_rat.has_value());
    assert(res_rat.value() == Rational(1, 2));

    auto res_rat2 = GmpEvaluator::eval_rational("(3/4 * 8/9) - 1/6");
    assert(res_rat2.has_value());
    assert(res_rat2.value() == Rational(1, 2));
    std::cout << "  Passed GmpEvaluator!\n";
}

static void test_vm_builtins() {
    std::cout << "[Test] Bytecode VM GMP built-in functions...\n";

    auto expr_gcd = formulaic::Expression::parse("gcd(100, 35)");
    assert(expr_gcd.has_value());
    assert(std::abs(expr_gcd.value().eval() - 5.0) < 1e-6);

    auto expr_lcm = formulaic::Expression::parse("lcm(12, 18)");
    assert(expr_lcm.has_value());
    assert(std::abs(expr_lcm.value().eval() - 36.0) < 1e-6);

    auto expr_fact = formulaic::Expression::parse("fact(5)");
    assert(expr_fact.has_value());
    assert(std::abs(expr_fact.value().eval() - 120.0) < 1e-6);

    auto expr_bin = formulaic::Expression::parse("binomial(6, 2)");
    assert(expr_bin.has_value());
    assert(std::abs(expr_bin.value().eval() - 15.0) < 1e-6);

    auto expr_fib = formulaic::Expression::parse("fib(10)");
    assert(expr_fib.has_value());
    assert(std::abs(expr_fib.value().eval() - 55.0) < 1e-6);
    std::cout << "  Passed Bytecode VM GMP built-in functions!\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Running Formulaic GMP Integration Tests \n";
    std::cout << "========================================\n";

    test_bigint_basics();
    test_number_theory();
    test_rational_basics();
    test_gmp_evaluator();
    test_vm_builtins();

    std::cout << "========================================\n";
    std::cout << "All GMP Tests PASSED successfully!      \n";
    std::cout << "========================================\n";
    return 0;
}
