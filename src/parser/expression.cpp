#include <Formulaic/parser/expression.hpp>
#include "bytecode_compiler.hpp"
#include "bytecode_vm.hpp"
#include "lexer.hpp"
#include "parser.hpp"

namespace formulaic {

Expression::Expression(std::string source_expr, BytecodeProgram program)
    : source_(std::move(source_expr)), program_(std::move(program)) {}

Result<Expression> Expression::parse(
    std::string_view expression_text,
    const std::vector<std::string>& variable_names
) {
    Lexer lexer(expression_text);
    auto tokens_res = lexer.tokenize();
    if (!tokens_res) {
        return tokens_res.error();
    }

    Parser parser(std::move(tokens_res.value()), variable_names);
    auto ast_res = parser.parse();
    if (!ast_res) {
        return ast_res.error();
    }

    BytecodeCompiler compiler(variable_names);
    auto program_res = compiler.compile(*ast_res.value());
    if (!program_res) {
        return program_res.error();
    }

    return Expression(std::string(expression_text), std::move(program_res.value()));
}

double Expression::evaluate(std::span<const double> variables) const noexcept {
    if (program_.instructions.empty()) return 0.0;
    return BytecodeVM::evaluate(program_, variables);
}

void Expression::evaluate_grid(
    double x_min, double x_max, size_t x_count,
    double y_min, double y_max, size_t y_count,
    double time_t,
    double* out_buffer
) const noexcept {
    if (!out_buffer || x_count == 0 || y_count == 0 || program_.instructions.empty()) return;

    const double dx = (x_count > 1) ? (x_max - x_min) / (x_count - 1) : 0.0;
    const double dy = (y_count > 1) ? (y_max - y_min) / (y_count - 1) : 0.0;

    double vars[3];
    vars[2] = time_t;

    for (size_t yi = 0; yi < y_count; ++yi) {
        vars[1] = y_min + yi * dy;
        const size_t row_offset = yi * x_count;
        for (size_t xi = 0; xi < x_count; ++xi) {
            vars[0] = x_min + xi * dx;
            out_buffer[row_offset + xi] = BytecodeVM::evaluate(program_, std::span<const double, 3>(vars));
        }
    }
}

} // namespace formulaic
