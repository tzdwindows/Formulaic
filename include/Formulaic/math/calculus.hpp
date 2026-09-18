#pragma once

#include <Formulaic/core/export.hpp>
#include <Formulaic/parser/expression.hpp>
#include <functional>
#include <utility>

namespace formulaic::math {

class FORMULAIC_API Calculus {
public:
    // 5-point stencil numerical differentiation:
    // f'(x) = (-f(x + 2h) + 8f(x + h) - 8f(x - h) + f(x - 2h)) / (12h) + O(h^4)
    [[nodiscard]] static double differentiate(
        const Expression& expr,
        double x,
        double h = 1e-5
    ) noexcept;

    [[nodiscard]] static double differentiate_y(
        const Expression& expr,
        double x,
        double y,
        double h = 1e-5
    ) noexcept;

    [[nodiscard]] static double differentiate_fn(
        const std::function<double(double)>& fn,
        double x,
        double h = 1e-5
    );

    // Second order central difference:
    // f''(x) = (-f(x+2h) + 16f(x+h) - 30f(x) + 16f(x-h) - f(x-2h)) / (12h^2)
    [[nodiscard]] static double second_derivative(
        const Expression& expr,
        double x,
        double h = 1e-4
    ) noexcept;

    // 2D Gradient: [df/dx, df/dy]
    [[nodiscard]] static std::pair<double, double> gradient(
        const Expression& expr,
        double x,
        double y,
        double h = 1e-5
    ) noexcept;

    // Composite Simpson's 1/3 Rule for numerical integration:
    // int_a^b f(x) dx = (h/3) * [f(x0) + 4*sum(f(x_odd)) + 2*sum(f(x_even)) + f(xn)]
    [[nodiscard]] static double integrate(
        const Expression& expr,
        double a,
        double b,
        size_t steps = 1000
    ) noexcept;

    [[nodiscard]] static double integrate_fn(
        const std::function<double(double)>& fn,
        double a,
        double b,
        size_t steps = 1000
    );

    // Trapezoidal rule integration
    [[nodiscard]] static double integrate_trapz(
        const Expression& expr,
        double a,
        double b,
        size_t steps = 1000
    ) noexcept;
};

} // namespace formulaic::math
