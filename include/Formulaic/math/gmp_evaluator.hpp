#pragma once

#include <Formulaic/core/error.hpp>
#include <Formulaic/core/export.hpp>
#include <Formulaic/math/bigint.hpp>
#include <Formulaic/math/rational.hpp>
#include <string_view>

namespace formulaic::math {

class FORMULAIC_API GmpEvaluator {
public:
    // Evaluate integer expression with arbitrary precision (supports +, -, *, /, %, ^, !, gcd, lcm, fact, bin, fib, sqrt)
    [[nodiscard]] static Result<BigInt> eval_int(std::string_view expr_str);

    // Evaluate exact rational expression with arbitrary precision
    [[nodiscard]] static Result<Rational> eval_rational(std::string_view expr_str);
};

} // namespace formulaic::math
