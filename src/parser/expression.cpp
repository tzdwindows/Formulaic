#include <Formulaic/parser/expression.hpp>
#include <Formulaic/math/calculus.hpp>
#include <Formulaic/math/fft.hpp>
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

Result<Expression> Expression::parse_equation(
    std::string_view equation_text,
    const std::vector<std::string>& variable_names
) {
    int paren_depth = 0;
    size_t eq_pos = std::string_view::npos;
    size_t eq_len = 0;

    for (size_t i = 0; i < equation_text.size(); ++i) {
        char c = equation_text[i];
        if (c == '(') {
            ++paren_depth;
        } else if (c == ')') {
            if (paren_depth > 0) --paren_depth;
        } else if (paren_depth == 0) {
            if (c == '=' && i > 0 && (equation_text[i-1] == '!' || equation_text[i-1] == '<' || equation_text[i-1] == '>')) {
                continue;
            }
            if (c == '=') {
                eq_pos = i;
                eq_len = (i + 1 < equation_text.size() && equation_text[i+1] == '=') ? 2 : 1;
                break;
            }
        }
    }

    if (eq_pos == std::string_view::npos) {
        return parse(equation_text, variable_names);
    }

    auto trim = [](std::string_view s) -> std::string_view {
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) {
            s.remove_prefix(1);
        }
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n' || s.back() == ';')) {
            s.remove_suffix(1);
        }
        return s;
    };

    std::string_view lhs = trim(equation_text.substr(0, eq_pos));
    std::string_view rhs = trim(equation_text.substr(eq_pos + eq_len));

    if (lhs.empty() || rhs.empty()) {
        return parse(equation_text, variable_names);
    }

    std::string implicit_expr = "(" + std::string(lhs) + ") - (" + std::string(rhs) + ")";
    return parse(implicit_expr, variable_names);
}

Result<Expression> Expression::parse_latex(
    std::string_view latex_text,
    const std::vector<std::string>& variable_names
) {
    auto script_res = LatexConverter::to_script(latex_text);
    if (!script_res) {
        return script_res.error();
    }
    const std::string& script = script_res.value();
    size_t last_eq = script.find('=');
    if (last_eq != std::string::npos && script.find("let ") == std::string::npos && script.find("var ") == std::string::npos) {
        return parse_equation(script, variable_names);
    }
    return parse(script, variable_names);
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

double Expression::differentiate(double x, double h) const noexcept {
    return math::Calculus::differentiate(*this, x, h);
}

double Expression::integrate(double a, double b, size_t steps) const noexcept {
    return math::Calculus::integrate(*this, a, b, steps);
}

std::vector<double> Expression::compute_spectrum(double t_start, double t_end, size_t sample_count) const {
    return math::FFT::sample_and_spectrum(*this, t_start, t_end, sample_count);
}

bool Expression::references_variable(std::string_view name) const noexcept {
    const size_t var_count = program_.variable_names.size();
    for (const auto& inst : program_.instructions) {
        if (inst.op == Opcode::LOAD_VAR && inst.operand < var_count) {
            if (program_.variable_names[inst.operand] == name) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string> Expression::referenced_variables() const {
    std::vector<std::string> result;
    const size_t var_count = program_.variable_names.size();
    for (const auto& inst : program_.instructions) {
        if (inst.op == Opcode::LOAD_VAR && inst.operand < var_count) {
            const auto& var_name = program_.variable_names[inst.operand];
            if (std::find(result.begin(), result.end(), var_name) == result.end()) {
                result.push_back(var_name);
            }
        }
    }
    return result;
}

Result<std::string> Expression::to_latex(const LatexFormatOptions& options) const {
    return LatexConverter::convert(source_, options, program_.variable_names);
}

} // namespace formulaic
