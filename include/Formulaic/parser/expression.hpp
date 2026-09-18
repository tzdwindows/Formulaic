#pragma once

#include <Formulaic/core/error.hpp>
#include <Formulaic/core/export.hpp>
#include <Formulaic/parser/bytecode.hpp>
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
        const std::vector<std::string>& variable_names = {"x", "y", "t"}
    );

    // High frequency evaluation with zero heap allocation
    [[nodiscard]] double evaluate(std::span<const double> variables) const noexcept;

    // Convenience evaluators for 1, 2, and 3 variables (e.g. x, y, t)
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

    [[nodiscard]] const std::string& source() const noexcept { return source_; }
    [[nodiscard]] const std::vector<std::string>& variables() const noexcept { return program_.variable_names; }
    [[nodiscard]] const BytecodeProgram& bytecode() const noexcept { return program_; }
    [[nodiscard]] std::string disassemble() const { return program_.disassemble(); }
    [[nodiscard]] bool is_valid() const noexcept { return !program_.instructions.empty(); }

private:
    std::string source_;
    BytecodeProgram program_;
};

} // namespace formulaic
