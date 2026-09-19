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
    std::string text(equation_text);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r' || text.back() == '\n' || text.back() == ';')) {
        text.pop_back();
    }

    size_t last_semi = std::string::npos;
    int paren_cnt = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '(') ++paren_cnt;
        else if (text[i] == ')') { if (paren_cnt > 0) --paren_cnt; }
        else if (text[i] == ';' && paren_cnt == 0) last_semi = i;
    }

    std::string prefix_stmts = (last_semi != std::string::npos) ? text.substr(0, last_semi + 1) + "\n" : "";
    std::string last_stmt = (last_semi != std::string::npos) ? text.substr(last_semi + 1) : text;

    size_t s_idx = 0;
    while (s_idx < last_stmt.size() && (last_stmt[s_idx] == ' ' || last_stmt[s_idx] == '\t' || last_stmt[s_idx] == '\r' || last_stmt[s_idx] == '\n')) ++s_idx;
    std::string_view prefix = std::string_view(last_stmt).substr(s_idx, 4);

    bool is_eq = false;
    std::string eq_lhs, eq_rhs;
    if (prefix != "let " && prefix != "var ") {
        int p_depth = 0;
        for (size_t p = 0; p < last_stmt.size(); ++p) {
            if (last_stmt[p] == '(') ++p_depth;
            else if (last_stmt[p] == ')') { if (p_depth > 0) --p_depth; }
            else if (p_depth == 0 && last_stmt[p] == '=') {
                if (p > 0 && (last_stmt[p-1] == '!' || last_stmt[p-1] == '<' || last_stmt[p-1] == '>')) continue;
                size_t eq_len = (p + 1 < last_stmt.size() && last_stmt[p+1] == '=') ? 2 : 1;
                eq_lhs = last_stmt.substr(0, p);
                eq_rhs = last_stmt.substr(p + eq_len);
                is_eq = true;
                break;
            }
        }
    }

    if (is_eq) {
        auto trim_str = [](std::string s) -> std::string {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.erase(0, 1);
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n' || s.back() == ';')) s.pop_back();
            return s;
        };
        eq_lhs = trim_str(eq_lhs);
        eq_rhs = trim_str(eq_rhs);

        if (eq_lhs == "y" || eq_lhs == "z") {
            auto rhs_res = parse(prefix_stmts + eq_rhs + ";", variable_names);
            if (rhs_res.has_value() && !rhs_res->references_variable(eq_lhs)) {
                return rhs_res;
            }
        }

        std::string implicit_expr = prefix_stmts + "(" + eq_lhs + ") - (" + eq_rhs + ");";
        return parse(implicit_expr, variable_names);
    }

    return parse(equation_text, variable_names);
}

Result<Expression> Expression::parse_latex(
    std::string_view latex_text,
    const std::vector<std::string>& variable_names
) {
    auto script_res = LatexConverter::to_script(latex_text);
    if (!script_res) {
        return script_res.error();
    }
    return parse_equation(script_res.value(), variable_names);
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
