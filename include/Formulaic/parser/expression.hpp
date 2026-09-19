#pragma once

#include <Formulaic/core/error.hpp>
#include <Formulaic/core/export.hpp>
#include <Formulaic/parser/bytecode.hpp>
#include <Formulaic/parser/latex_converter.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace formulaic {

class FORMULAIC_API Expression {
public:
    Expression() = default;
    explicit Expression(std::string source_expr, BytecodeProgram program);

    [[nodiscard]] static Result<Expression> parse(
        std::string_view expression_text,
        const std::vector<std::string>& variable_names = {"x", "y", "t", "z"}
    );

    // Parses mathematical equation LHS = RHS or LHS == RHS into an implicit function (LHS) - (RHS)
    [[nodiscard]] static Result<Expression> parse_equation(
        std::string_view equation_text,
        const std::vector<std::string>& variable_names = {"x", "y", "t", "z"}
    );

    // Parses a standard LaTeXLive formula directly into an executable Expression
    [[nodiscard]] static Result<Expression> parse_latex(
        std::string_view latex_text,
        const std::vector<std::string>& variable_names = {"x", "y", "t", "z", "r", "u", "v", "h"}
    );

    // High frequency evaluation with zero heap allocation
    [[nodiscard]] double evaluate(std::span<const double> variables) const noexcept;

    // Convenience evaluators for 0, 1, 2, and 3 variables (e.g. x, y, t)
    [[nodiscard]] double eval() const noexcept {
        return evaluate({});
    }

    [[nodiscard]] double eval(double x) const noexcept {
        const double vars[1] = {x};
        return evaluate(std::span<const double, 1>(vars));
    }

    [[nodiscard]] double eval(double x, double y) const noexcept {
        const double vars[2] = {x, y};
        return evaluate(std::span<const double, 2>(vars));
    }

    [[nodiscard]] double eval(double x, double y, double t) const noexcept {
        const double vars[3] = {x, y, t};
        return evaluate(std::span<const double, 3>(vars));
    }

    // High performance batch evaluation over contiguous buffers
    void evaluate_grid(
        double x_min, double x_max, size_t x_count,
        double y_min, double y_max, size_t y_count,
        double time_t,
        double* out_buffer
    ) const noexcept;

    // Advanced Calculus & Spectral Analysis Extensions
    [[nodiscard]] double differentiate(double x, double h = 1e-5) const noexcept;
    [[nodiscard]] double integrate(double a, double b, size_t steps = 1000) const noexcept;
    [[nodiscard]] std::vector<double> compute_spectrum(double t_start, double t_end, size_t sample_count) const;

    // Convert current expression to standard LaTeXLive formula
    [[nodiscard]] Result<std::string> to_latex(const LatexFormatOptions& options = {}) const;

    [[nodiscard]] const std::string& source() const noexcept { return source_; }
    [[nodiscard]] const std::vector<std::string>& variables() const noexcept { return program_.variable_names; }
    [[nodiscard]] bool references_variable(std::string_view name) const noexcept;
    [[nodiscard]] std::vector<std::string> referenced_variables() const;
    [[nodiscard]] const BytecodeProgram& bytecode() const noexcept { return program_; }
    [[nodiscard]] std::string disassemble() const { return program_.disassemble(); }
    [[nodiscard]] bool is_valid() const noexcept { return !program_.instructions.empty(); }

private:
    std::string source_;
    BytecodeProgram program_;
};

} // namespace formulaic
