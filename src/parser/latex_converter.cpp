#include <Formulaic/parser/latex_converter.hpp>
#include "lexer.hpp"
#include "parser.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace formulaic {

namespace {

const std::unordered_map<std::string, std::string>& get_greek_table() {
    static const std::unordered_map<std::string, std::string> kGreek = {
        {"alpha", "\\alpha"},
        {"beta", "\\beta"},
        {"gamma", "\\gamma"},
        {"delta", "\\delta"},
        {"epsilon", "\\varepsilon"},
        {"zeta", "\\zeta"},
        {"eta", "\\eta"},
        {"theta", "\\theta"},
        {"iota", "\\iota"},
        {"kappa", "\\kappa"},
        {"lambda", "\\lambda"},
        {"mu", "\\mu"},
        {"nu", "\\nu"},
        {"xi", "\\xi"},
        {"pi", "\\pi"},
        {"rho", "\\rho"},
        {"sigma", "\\sigma"},
        {"tau", "\\tau"},
        {"upsilon", "\\upsilon"},
        {"phi", "\\phi"},
        {"chi", "\\chi"},
        {"psi", "\\psi"},
        {"omega", "\\omega"},
        {"Gamma", "\\Gamma"},
        {"Delta", "\\Delta"},
        {"Theta", "\\Theta"},
        {"Lambda", "\\Lambda"},
        {"Xi", "\\Xi"},
        {"Pi", "\\Pi"},
        {"Sigma", "\\Sigma"},
        {"Upsilon", "\\Upsilon"},
        {"Phi", "\\Phi"},
        {"Psi", "\\Psi"},
        {"Omega", "\\Omega"}
    };
    return kGreek;
}

int get_node_precedence(const ASTNode& node) {
    if (const auto* bin = dynamic_cast<const BinaryOpNode*>(&node)) {
        switch (bin->op) {
            case BinaryOp::LogicalOr: return 1;
            case BinaryOp::LogicalAnd: return 2;
            case BinaryOp::Equal:
            case BinaryOp::NotEqual:
            case BinaryOp::Less:
            case BinaryOp::LessEqual:
            case BinaryOp::Greater:
            case BinaryOp::GreaterEqual: return 3;
            case BinaryOp::Add:
            case BinaryOp::Subtract: return 4;
            case BinaryOp::Multiply:
            case BinaryOp::Modulo: return 5;
            case BinaryOp::Divide: return 5;
            case BinaryOp::Power: return 6;
        }
    }
    if (dynamic_cast<const UnaryOpNode*>(&node)) {
        return 7;
    }
    return 100;
}

std::string format_number(double val) {
    if (std::isnan(val)) return "\\text{NaN}";
    if (std::isinf(val)) return (val > 0) ? "\\infty" : "-\\infty";

    if (val == std::floor(val) && std::abs(val) < 1e14) {
        return std::to_string(static_cast<long long>(val));
    }

    std::ostringstream ss;
    ss.precision(6);
    ss << val;
    std::string s = ss.str();
    return s;
}

std::string format_variable(const std::string& name, const LatexFormatOptions& opt) {
    if (opt.greek_translation) {
        const auto& greek = get_greek_table();
        auto it = greek.find(name);
        if (it != greek.end()) {
            return it->second;
        }
    }

    // Check single letter + digits: e.g. u0 -> u_{0}, x1 -> x_{1}
    if (name.size() >= 2 && std::isalpha(static_cast<unsigned char>(name[0]))) {
        bool all_digits = true;
        for (size_t i = 1; i < name.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(name[i]))) {
                all_digits = false;
                break;
            }
        }
        if (all_digits) {
            return std::string(1, name[0]) + "_{" + name.substr(1) + "}";
        }
    }

    // Check underscore: e.g. lap_u -> \text{lap}_{u}
    size_t underscore = name.find('_');
    if (underscore != std::string::npos) {
        std::string base = name.substr(0, underscore);
        std::string sub = name.substr(underscore + 1);
        std::string clean_sub;
        for (char c : sub) {
            if (c == '_') clean_sub += "\\_";
            else clean_sub += c;
        }
        std::string base_fmt = (base.size() == 1) ? base : "\\text{" + base + "}";
        return base_fmt + "_{" + clean_sub + "}";
    }

    // Single letter variables
    if (name.size() == 1) {
        return name;
    }

    // Known common mathematical constants
    if (name == "pi") return "\\pi";
    if (name == "inf" || name == "infinity") return "\\infty";

    // Multi-letter word variable
    return "\\text{" + name + "}";
}

class LatexAstVisitor {
public:
    explicit LatexAstVisitor(const LatexFormatOptions& opt) : opt_(opt) {}

    std::string visit(const ASTNode& node) {
        if (const auto* num = dynamic_cast<const NumberNode*>(&node)) {
            return visit_number(*num);
        }
        if (const auto* var = dynamic_cast<const VariableNode*>(&node)) {
            return visit_variable(*var);
        }
        if (const auto* unary = dynamic_cast<const UnaryOpNode*>(&node)) {
            return visit_unary(*unary);
        }
        if (const auto* binary = dynamic_cast<const BinaryOpNode*>(&node)) {
            return visit_binary(*binary);
        }
        if (const auto* fn = dynamic_cast<const FunctionCallNode*>(&node)) {
            return visit_function(*fn);
        }
        if (const auto* decl = dynamic_cast<const VarDeclNode*>(&node)) {
            return visit_decl(*decl);
        }
        if (const auto* assign = dynamic_cast<const AssignmentNode*>(&node)) {
            return visit_assign(*assign);
        }
        if (const auto* block = dynamic_cast<const BlockNode*>(&node)) {
            return visit_block(*block);
        }
        return "";
    }

private:
    LatexFormatOptions opt_;

    std::string visit_number(const NumberNode& node) {
        return format_number(node.value);
    }

    std::string visit_variable(const VariableNode& node) {
        return format_variable(node.name, opt_);
    }

    std::string visit_unary(const UnaryOpNode& node) {
        if (!node.operand) return "";
        std::string sub = visit(*node.operand);
        int sub_prec = get_node_precedence(*node.operand);

        if (node.op == UnaryOp::Negate) {
            if (sub_prec <= 4) {
                return "-\\left(" + sub + "\\right)";
            }
            return "-" + sub;
        } else if (node.op == UnaryOp::Not) {
            if (sub_prec <= 4) {
                return "\\neg\\left(" + sub + "\\right)";
            }
            return "\\neg " + sub;
        }
        return sub;
    }

    std::string visit_binary(const BinaryOpNode& node) {
        if (!node.left || !node.right) return "";

        int cur_prec = get_node_precedence(node);
        int l_prec = get_node_precedence(*node.left);
        int r_prec = get_node_precedence(*node.right);

        // Division: \frac{num}{den}
        if (node.op == BinaryOp::Divide && opt_.pretty_fractions) {
            std::string num = visit(*node.left);
            std::string den = visit(*node.right);
            return "\\frac{" + num + "}{" + den + "}";
        }

        // Power: base^{exp}
        if (node.op == BinaryOp::Power) {
            std::string base;
            // Check if base is function call like sin(x)
            if (const auto* fn = dynamic_cast<const FunctionCallNode*>(node.left.get())) {
                if (is_trig_or_hyperbolic(fn->name) && fn->args.size() == 1) {
                    std::string arg = visit(*fn->args[0]);
                    std::string exp = visit(*node.right);
                    return get_fn_latex_name(fn->name) + "^{" + exp + "}\\left(" + arg + "\\right)";
                }
            }

            if (l_prec < 6) {
                base = "\\left(" + visit(*node.left) + "\\right)";
            } else {
                base = visit(*node.left);
            }
            std::string exp = visit(*node.right);
            return base + "^{" + exp + "}";
        }

        std::string l_str = visit(*node.left);
        std::string r_str = visit(*node.right);

        // Parentheses formatting based on operator precedence
        if (l_prec < cur_prec) {
            l_str = "\\left(" + l_str + "\\right)";
        }

        if (node.op == BinaryOp::Subtract) {
            // a - (b + c) or a - (b - c)
            if (r_prec <= cur_prec) {
                r_str = "\\left(" + r_str + "\\right)";
            }
        } else if (node.op == BinaryOp::Multiply || node.op == BinaryOp::Modulo) {
            if (r_prec < cur_prec) {
                r_str = "\\left(" + r_str + "\\right)";
            }
        } else {
            if (r_prec < cur_prec) {
                r_str = "\\left(" + r_str + "\\right)";
            }
        }

        switch (node.op) {
            case BinaryOp::Add: return l_str + " + " + r_str;
            case BinaryOp::Subtract: return l_str + " - " + r_str;
            case BinaryOp::Multiply: return l_str + " \\cdot " + r_str;
            case BinaryOp::Divide: return "\\frac{" + l_str + "}{" + r_str + "}";
            case BinaryOp::Modulo: return l_str + " \\bmod " + r_str;
            case BinaryOp::Equal: return l_str + " = " + r_str;
            case BinaryOp::NotEqual: return l_str + " \\neq " + r_str;
            case BinaryOp::Less: return l_str + " < " + r_str;
            case BinaryOp::LessEqual: return l_str + " \\leq " + r_str;
            case BinaryOp::Greater: return l_str + " > " + r_str;
            case BinaryOp::GreaterEqual: return l_str + " \\geq " + r_str;
            case BinaryOp::LogicalAnd: return l_str + " \\land " + r_str;
            case BinaryOp::LogicalOr: return l_str + " \\lor " + r_str;
            default: return l_str + " ? " + r_str;
        }
    }

    std::string visit_function(const FunctionCallNode& node) {
        const auto& name = node.name;
        const auto& args = node.args;

        if (name == "sqrt" && args.size() == 1) {
            return "\\sqrt{" + visit(*args[0]) + "}";
        }
        if (name == "cbrt" && args.size() == 1) {
            return "\\sqrt[3]{" + visit(*args[0]) + "}";
        }
        if (name == "hypot" && args.size() == 2) {
            return "\\sqrt{" + visit(*args[0]) + "^{2} + " + visit(*args[1]) + "^{2}}";
        }
        if (name == "abs" && args.size() == 1) {
            return "\\left| " + visit(*args[0]) + " \\right|";
        }
        if (name == "floor" && args.size() == 1) {
            return "\\left\\lfloor " + visit(*args[0]) + " \\right\\rfloor";
        }
        if (name == "ceil" && args.size() == 1) {
            return "\\left\\lceil " + visit(*args[0]) + " \\right\\rceil";
        }
        if ((name == "bin" || name == "binomial" || name == "ncr") && args.size() == 2) {
            return "\\binom{" + visit(*args[0]) + "}{" + visit(*args[1]) + "}";
        }
        if ((name == "fact" || name == "factorial") && args.size() == 1) {
            int p = get_node_precedence(*args[0]);
            if (p < 7) {
                return "\\left(" + visit(*args[0]) + "\\right)!";
            }
            return visit(*args[0]) + "!";
        }
        if ((name == "fib" || name == "fibonacci") && args.size() == 1) {
            return "F_{" + visit(*args[0]) + "}";
        }
        if (name == "exp" && args.size() == 1) {
            std::string a = visit(*args[0]);
            return "e^{" + a + "}";
        }
        if (name == "exp2" && args.size() == 1) {
            return "2^{" + visit(*args[0]) + "}";
        }
        if (name == "log10" && args.size() == 1) {
            return "\\log_{10}\\left(" + visit(*args[0]) + "\\right)";
        }
        if (name == "log2" && args.size() == 1) {
            return "\\log_{2}\\left(" + visit(*args[0]) + "\\right)";
        }
        if ((name == "gamma" || name == "tgamma") && args.size() == 1) {
            return "\\Gamma\\left(" + visit(*args[0]) + "\\right)";
        }
        if (name == "beta" && args.size() == 2) {
            return "\\mathrm{B}\\left(" + visit(*args[0]) + ", " + visit(*args[1]) + "\\right)";
        }

        // Standard functions
        std::string fn_tex = get_fn_latex_name(name);
        std::string args_tex;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) args_tex += ", ";
            args_tex += visit(*args[i]);
        }
        return fn_tex + "\\left(" + args_tex + "\\right)";
    }

    std::string visit_decl(const VarDeclNode& node) {
        std::string var_tex = format_variable(node.name, opt_);
        std::string init_tex = node.initializer ? visit(*node.initializer) : "0";
        return var_tex + " &= " + init_tex;
    }

    std::string visit_assign(const AssignmentNode& node) {
        std::string var_tex = format_variable(node.name, opt_);
        std::string val_tex = node.value ? visit(*node.value) : "0";
        return var_tex + " &= " + val_tex;
    }

    std::string visit_block(const BlockNode& node) {
        if (node.statements.empty() && node.result_expr) {
            return visit(*node.result_expr);
        }

        std::vector<std::string> lines;
        for (const auto& stmt : node.statements) {
            lines.push_back(visit(*stmt));
        }
        if (node.result_expr) {
            std::string res = visit(*node.result_expr);
            if (res.find("&=") == std::string::npos) {
                res = "f(x, y) &= " + res;
            }
            lines.push_back(res);
        }

        if (lines.size() == 1) {
            return lines[0];
        }

        if (opt_.aligned_multiline) {
            std::string out = "\\begin{aligned}\n";
            for (size_t i = 0; i < lines.size(); ++i) {
                out += "  " + lines[i];
                if (i + 1 < lines.size()) out += " \\\\";
                out += "\n";
            }
            out += "\\end{aligned}";
            return out;
        } else {
            std::string out;
            for (size_t i = 0; i < lines.size(); ++i) {
                if (i > 0) out += "\n";
                out += lines[i];
            }
            return out;
        }
    }

    static bool is_trig_or_hyperbolic(const std::string& name) {
        static const std::unordered_set<std::string> kTrig = {
            "sin", "cos", "tan", "sec", "csc", "cot",
            "sinh", "cosh", "tanh", "sech", "csch", "coth"
        };
        return kTrig.contains(name);
    }

    static std::string get_fn_latex_name(const std::string& name) {
        static const std::unordered_map<std::string, std::string> kMap = {
            {"sin", "\\sin"},
            {"cos", "\\cos"},
            {"tan", "\\tan"},
            {"asin", "\\arcsin"},
            {"arcsin", "\\arcsin"},
            {"acos", "\\arccos"},
            {"arccos", "\\arccos"},
            {"atan", "\\arctan"},
            {"arctan", "\\arctan"},
            {"atan2", "\\operatorname{atan2}"},
            {"sinh", "\\sinh"},
            {"cosh", "\\cosh"},
            {"tanh", "\\tanh"},
            {"sec", "\\sec"},
            {"csc", "\\csc"},
            {"cot", "\\cot"},
            {"sech", "\\operatorname{sech}"},
            {"csch", "\\operatorname{csch}"},
            {"coth", "\\coth"},
            {"asinh", "\\operatorname{arsinh}"},
            {"arsinh", "\\operatorname{arsinh}"},
            {"acosh", "\\operatorname{arcosh}"},
            {"arcosh", "\\operatorname{arcosh}"},
            {"atanh", "\\operatorname{artanh}"},
            {"artanh", "\\operatorname{artanh}"},
            {"ln", "\\ln"},
            {"log", "\\log"},
            {"sinc", "\\operatorname{sinc}"},
            {"erf", "\\operatorname{erf}"},
            {"erfc", "\\operatorname{erfc}"},
            {"sign", "\\operatorname{sgn}"},
            {"gcd", "\\gcd"},
            {"lcm", "\\operatorname{lcm}"},
            {"min", "\\min"},
            {"max", "\\max"},
            {"clamp", "\\operatorname{clamp}"},
            {"lerp", "\\operatorname{lerp}"},
            {"mix", "\\operatorname{mix}"},
            {"step", "\\operatorname{step}"},
            {"smoothstep", "\\operatorname{smoothstep}"},
            {"diff_step", "\\operatorname{diff\\_step}"},
            {"diff2_step", "\\operatorname{diff2\\_step}"},
            {"hann", "\\operatorname{hann}"},
            {"hamming", "\\operatorname{hamming}"},
            {"blackman", "\\operatorname{blackman}"},
            {"bartlett", "\\operatorname{bartlett}"},
            {"flattop", "\\operatorname{flattop}"},
            {"square_wave", "\\operatorname{square\\_wave}"},
            {"triangle_wave", "\\operatorname{tri\\_wave}"},
            {"sawtooth_wave", "\\operatorname{saw\\_wave}"},
            {"chirp", "\\operatorname{chirp}"}
        };
        auto it = kMap.find(name);
        if (it != kMap.end()) return it->second;

        std::string escaped;
        for (char c : name) {
            if (c == '_') escaped += "\\_";
            else escaped += c;
        }
        return "\\operatorname{" + escaped + "}";
    }
};

} // namespace

std::string LatexConverter::to_latex(const ASTNode& node, const LatexFormatOptions& options) {
    LatexAstVisitor visitor(options);
    std::string res = visitor.visit(node);
    if (options.display_math_delimiters) {
        return "\\[\n" + res + "\n\\]";
    }
    return res;
}

Result<std::string> LatexConverter::convert(
    std::string_view expression_text,
    const LatexFormatOptions& options,
    const std::vector<std::string>& variable_names
) {
    // 1. Check if expression contains '=' equation (outside parentheses)
    std::string trimmed(expression_text);
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t' || trimmed.back() == '\r' || trimmed.back() == '\n' || trimmed.back() == ';')) {
        trimmed.pop_back();
    }
    while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t' || trimmed.front() == '\r' || trimmed.front() == '\n')) {
        trimmed.erase(0, 1);
    }

    if (trimmed.empty()) {
        return std::string("");
    }

    // Check single-line or multi-line equation
    size_t last_semi = std::string::npos;
    int paren_cnt = 0;
    for (size_t i = 0; i < trimmed.size(); ++i) {
        if (trimmed[i] == '(') ++paren_cnt;
        else if (trimmed[i] == ')') { if (paren_cnt > 0) --paren_cnt; }
        else if (trimmed[i] == ';' && paren_cnt == 0) last_semi = i;
    }

    std::string prefix_stmts = (last_semi != std::string::npos) ? trimmed.substr(0, last_semi + 1) : "";
    std::string last_stmt = (last_semi != std::string::npos) ? trimmed.substr(last_semi + 1) : trimmed;

    // Check for equality sign in last statement
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

    // Auto-discover non-function identifiers to treat as variables
    std::unordered_set<std::string> var_set(variable_names.begin(), variable_names.end());
    Lexer pre_lexer(expression_text);
    if (auto pre_tokens = pre_lexer.tokenize()) {
        const auto& toks = pre_tokens.value();
        for (size_t i = 0; i < toks.size(); ++i) {
            if (toks[i].type == TokenType::Identifier) {
                std::string id_str(toks[i].text);
                if (i + 1 < toks.size() && toks[i + 1].type == TokenType::LParen) {
                    continue; // Function call
                }
                if (id_str != "let" && id_str != "var") {
                    var_set.insert(id_str);
                }
            }
        }
    }
    std::vector<std::string> effective_vars(var_set.begin(), var_set.end());

    if (is_eq) {
        LatexFormatOptions inner_opt = options;
        inner_opt.display_math_delimiters = false;

        auto convert_half = [&](std::string_view s) -> Result<std::string> {
            Lexer lex(s);
            auto tok_res = lex.tokenize();
            if (!tok_res) return tok_res.error();
            Parser p(std::move(tok_res.value()), effective_vars);
            auto ast_res = p.parse();
            if (!ast_res) return ast_res.error();
            return to_latex(*ast_res.value(), inner_opt);
        };

        auto lhs_res = convert_half(eq_lhs);
        if (!lhs_res) return lhs_res.error();
        auto rhs_res = convert_half(eq_rhs);
        if (!rhs_res) return rhs_res.error();

        std::string eq_line = lhs_res.value() + " = " + rhs_res.value();

        if (prefix_stmts.empty()) {
            if (options.display_math_delimiters) {
                return "\\[\n" + eq_line + "\n\\]";
            }
            return eq_line;
        } else {
            // Combine with prefix declarations
            Lexer lex(prefix_stmts);
            auto tok_res = lex.tokenize();
            if (!tok_res) return tok_res.error();
            Parser p(std::move(tok_res.value()), effective_vars);
            auto ast_res = p.parse();
            if (!ast_res) return ast_res.error();

            std::string prefix_tex = to_latex(*ast_res.value(), inner_opt);
            if (options.aligned_multiline) {
                // Insert eq_line into aligned block
                std::string res_aligned = "\\begin{aligned}\n";
                if (const auto* blk = dynamic_cast<const BlockNode*>(ast_res.value().get())) {
                    for (const auto& stmt : blk->statements) {
                        res_aligned += "  " + to_latex(*stmt, inner_opt) + " \\\\\n";
                    }
                }
                res_aligned += "  " + lhs_res.value() + " &= " + rhs_res.value() + "\n\\end{aligned}";
                if (options.display_math_delimiters) {
                    return "\\[\n" + res_aligned + "\n\\]";
                }
                return res_aligned;
            }
            return prefix_tex + "\n" + eq_line;
        }
    }

    // Standard expression / script parsing
    Lexer lexer(expression_text);
    auto tokens_res = lexer.tokenize();
    if (!tokens_res) return tokens_res.error();

    Parser parser(std::move(tokens_res.value()), effective_vars);
    auto ast_res = parser.parse();
    if (!ast_res) return ast_res.error();

    std::string latex = to_latex(*ast_res.value(), options);

    if (options.include_y_equals) {
        // If single expression not starting with y =
        if (latex.find('=') == std::string::npos && latex.find("\\begin{aligned}") == std::string::npos) {
            latex = "y = " + latex;
        }
    }

    return latex;
}

} // namespace formulaic
