#include <Formulaic/math/calculus.hpp>
#include <cmath>

namespace formulaic::math {

double Calculus::differentiate(
    const Expression& expr,
    double x,
    double h
) noexcept {
    if (h == 0.0) return 0.0;

    const double fm2 = expr.eval(x - 2.0 * h);
    const double fm1 = expr.eval(x - h);
    const double fp1 = expr.eval(x + h);
    const double fp2 = expr.eval(x + 2.0 * h);

    return (-fp2 + 8.0 * fp1 - 8.0 * fm1 + fm2) / (12.0 * h);
}

double Calculus::differentiate_y(
    const Expression& expr,
    double x,
    double y,
    double h
) noexcept {
    if (h == 0.0) return 0.0;

    const double fm2 = expr.eval(x, y - 2.0 * h);
    const double fm1 = expr.eval(x, y - h);
    const double fp1 = expr.eval(x, y + h);
    const double fp2 = expr.eval(x, y + 2.0 * h);

    return (-fp2 + 8.0 * fp1 - 8.0 * fm1 + fm2) / (12.0 * h);
}

double Calculus::differentiate_fn(
    const std::function<double(double)>& fn,
    double x,
    double h
) {
    if (h == 0.0 || !fn) return 0.0;

    const double fm2 = fn(x - 2.0 * h);
    const double fm1 = fn(x - h);
    const double fp1 = fn(x + h);
    const double fp2 = fn(x + 2.0 * h);

    return (-fp2 + 8.0 * fp1 - 8.0 * fm1 + fm2) / (12.0 * h);
}

double Calculus::second_derivative(
    const Expression& expr,
    double x,
    double h
) noexcept {
    if (h == 0.0) return 0.0;

    const double fm2 = expr.eval(x - 2.0 * h);
    const double fm1 = expr.eval(x - h);
    const double f0  = expr.eval(x);
    const double fp1 = expr.eval(x + h);
    const double fp2 = expr.eval(x + 2.0 * h);

    return (-fp2 + 16.0 * fp1 - 30.0 * f0 + 16.0 * fm1 - fm2) / (12.0 * h * h);
}

std::pair<double, double> Calculus::gradient(
    const Expression& expr,
    double x,
    double y,
    double h
) noexcept {
    const double df_dx = differentiate(expr, x, h);
    const double df_dy = differentiate_y(expr, x, y, h);
    return {df_dx, df_dy};
}

double Calculus::integrate(
    const Expression& expr,
    double a,
    double b,
    size_t steps
) noexcept {
    if (a == b) return 0.0;
    if (steps < 2) steps = 2;
    if (steps % 2 != 0) ++steps; // Simpson's rule requires an even number of intervals

    const double h = (b - a) / static_cast<double>(steps);
    double sum = expr.eval(a) + expr.eval(b);

    for (size_t i = 1; i < steps; ++i) {
        const double x = a + static_cast<double>(i) * h;
        const double val = expr.eval(x);
        if (i % 2 == 1) {
            sum += 4.0 * val;
        } else {
            sum += 2.0 * val;
        }
    }

    return sum * (h / 3.0);
}

double Calculus::integrate_fn(
    const std::function<double(double)>& fn,
    double a,
    double b,
    size_t steps
) {
    if (!fn || a == b) return 0.0;
    if (steps < 2) steps = 2;
    if (steps % 2 != 0) ++steps;

    const double h = (b - a) / static_cast<double>(steps);
    double sum = fn(a) + fn(b);

    for (size_t i = 1; i < steps; ++i) {
        const double x = a + static_cast<double>(i) * h;
        const double val = fn(x);
        if (i % 2 == 1) {
            sum += 4.0 * val;
        } else {
            sum += 2.0 * val;
        }
    }

    return sum * (h / 3.0);
}

double Calculus::integrate_trapz(
    const Expression& expr,
    double a,
    double b,
    size_t steps
) noexcept {
    if (a == b) return 0.0;
    if (steps < 1) steps = 1;

    const double h = (b - a) / static_cast<double>(steps);
    double sum = 0.5 * (expr.eval(a) + expr.eval(b));

    for (size_t i = 1; i < steps; ++i) {
        const double x = a + static_cast<double>(i) * h;
        sum += expr.eval(x);
    }

    return sum * h;
}

} // namespace formulaic::math
