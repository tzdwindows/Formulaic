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

namespace {

std::string convert_latex_expr(std::string_view text);

void skip_spaces(std::string_view s, size_t& i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
        ++i;
    }
}

std::string extract_group(std::string_view s, size_t& i, char open_ch = '{', char close_ch = '}') {
    skip_spaces(s, i);
    if (i >= s.size()) return "";
    if (s[i] == open_ch) {
        ++i;
        int depth = 1;
        size_t start = i;
        while (i < s.size() && depth > 0) {
            if (s[i] == open_ch) ++depth;
            else if (s[i] == close_ch) {
                --depth;
                if (depth == 0) {
                    std::string res(s.substr(start, i - start));
                    ++i;
                    return res;
                }
            }
            ++i;
        }
        return std::string(s.substr(start));
    }
    if (s[i] == '\\') {
        size_t start = i++;
        while (i < s.size() && std::isalpha(static_cast<unsigned char>(s[i]))) ++i;
        return std::string(s.substr(start, i - start));
    }
    std::string res(1, s[i]);
    ++i;
    return res;
}

bool is_simple_atom(std::string_view s) {
    while (!s.empty() && s.front() == ' ') s.remove_prefix(1);
    while (!s.empty() && s.back() == ' ') s.remove_suffix(1);
    if (s.empty()) return false;
    for (char c : s) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.') {
            return false;
        }
    }
    return true;
}

const std::unordered_map<std::string, std::string>& get_latex_greek_to_name() {
    static const std::unordered_map<std::string, std::string> kMap = {
        {"\\alpha", "alpha"}, {"\\beta", "beta"}, {"\\gamma", "gamma"},
        {"\\delta", "delta"}, {"\\epsilon", "epsilon"}, {"\\varepsilon", "epsilon"},
        {"\\zeta", "zeta"}, {"\\eta", "eta"}, {"\\theta", "theta"},
        {"\\iota", "iota"}, {"\\kappa", "kappa"}, {"\\lambda", "lambda"},
        {"\\mu", "mu"}, {"\\nu", "nu"}, {"\\xi", "xi"},
        {"\\pi", "pi"}, {"\\rho", "rho"}, {"\\sigma", "sigma"},
        {"\\tau", "tau"}, {"\\upsilon", "upsilon"}, {"\\phi", "phi"},
        {"\\varphi", "phi"}, {"\\chi", "chi"}, {"\\psi", "psi"},
        {"\\omega", "omega"},
        {"\\Gamma", "Gamma"}, {"\\Delta", "Delta"}, {"\\Theta", "Theta"},
        {"\\Lambda", "Lambda"}, {"\\Xi", "Xi"}, {"\\Pi", "Pi"},
        {"\\Sigma", "Sigma"}, {"\\Upsilon", "Upsilon"}, {"\\Phi", "Phi"},
        {"\\Psi", "Psi"}, {"\\Omega", "Omega"}
    };
    return kMap;
}

const std::unordered_map<std::string, std::string>& get_latex_fn_to_name() {
    static const std::unordered_map<std::string, std::string> kFn = {
        {"\\sin", "sin"}, {"\\cos", "cos"}, {"\\tan", "tan"},
        {"\\asin", "asin"}, {"\\arcsin", "asin"},
        {"\\acos", "acos"}, {"\\arccos", "acos"},
        {"\\atan", "atan"}, {"\\arctan", "atan"},
        {"\\atan2", "atan2"},
        {"\\sec", "sec"}, {"\\csc", "csc"}, {"\\cot", "cot"},
        {"\\sinh", "sinh"}, {"\\cosh", "cosh"}, {"\\tanh", "tanh"},
        {"\\sech", "sech"}, {"\\csch", "csch"}, {"\\coth", "coth"},
        {"\\asinh", "asinh"}, {"\\arsinh", "asinh"},
        {"\\acosh", "acosh"}, {"\\arcosh", "acosh"},
        {"\\atanh", "atanh"}, {"\\artanh", "atanh"},
        {"\\ln", "ln"}, {"\\log", "log"},
        {"\\exp", "exp"}, {"\\sqrt", "sqrt"},
        {"\\cbrt", "cbrt"}, {"\\abs", "abs"},
        {"\\sinc", "sinc"}, {"\\erf", "erf"}, {"\\erfc", "erfc"},
        {"\\floor", "floor"}, {"\\ceil", "ceil"},
        {"\\min", "min"}, {"\\max", "max"},
        {"\\gcd", "gcd"}, {"\\lcm", "lcm"},
        {"\\clamp", "clamp"}, {"\\lerp", "lerp"},
        {"\\step", "step"}, {"\\smoothstep", "smoothstep"},
        {"\\sign", "sign"}, {"\\sgn", "sign"}
    };
    return kFn;
}

std::string convert_latex_expr(std::string_view text) {
    std::string out;
    size_t i = 0;
    const size_t n = text.size();

    auto starts_with = [&](std::string_view prefix) -> bool {
        return text.substr(i).starts_with(prefix);
    };

    while (i < n) {
        skip_spaces(text, i);
        if (i >= n) break;

        char c = text[i];

        // 1. Fractions: \frac{num}{den}, \dfrac, \tfrac
        if (starts_with("\\frac") || starts_with("\\dfrac") || starts_with("\\tfrac")) {
            i += (starts_with("\\frac") ? 5 : 6);
            std::string num = extract_group(text, i, '{', '}');
            std::string den = extract_group(text, i, '{', '}');
            std::string c_num = convert_latex_expr(num);
            std::string c_den = convert_latex_expr(den);
            std::string f_num = is_simple_atom(c_num) ? c_num : "(" + c_num + ")";
            std::string f_den = is_simple_atom(c_den) ? c_den : "(" + c_den + ")";
            if (!out.empty() && out.back() != ' ' && out.back() != '(' && out.back() != '+' && out.back() != '-' && out.back() != '*' && out.back() != '/' && out.back() != '=') {
                out += " ";
            }
            out += f_num + " / " + f_den;
            continue;
        }

        // 2. Square roots: \sqrt[n]{x} or \sqrt{x}
        if (starts_with("\\sqrt")) {
            i += 5;
            skip_spaces(text, i);
            std::string degree;
            if (i < n && text[i] == '[') {
                degree = extract_group(text, i, '[', ']');
            }
            std::string arg = extract_group(text, i, '{', '}');
            std::string c_arg = convert_latex_expr(arg);
            if (degree.empty()) {
                out += "sqrt(" + c_arg + ")";
            } else if (degree == "3") {
                out += "cbrt(" + c_arg + ")";
            } else {
                std::string c_deg = convert_latex_expr(degree);
                out += "(" + c_arg + ")^(1 / (" + c_deg + "))";
            }
            continue;
        }

        // 3. Binomial: \binom{n}{k}
        if (starts_with("\\binom")) {
            i += 6;
            std::string n_str = extract_group(text, i, '{', '}');
            std::string k_str = extract_group(text, i, '{', '}');
            out += "bin(" + convert_latex_expr(n_str) + ", " + convert_latex_expr(k_str) + ")";
            continue;
        }

        // 4. Paired delimiters: \left| ... \right|
        if (starts_with("\\left|")) {
            i += 6;
            size_t end_bar = text.find("\\right|", i);
            if (end_bar != std::string_view::npos) {
                std::string inner = std::string(text.substr(i, end_bar - i));
                i = end_bar + 7;
                out += "abs(" + convert_latex_expr(inner) + ")";
                continue;
            }
        }
        if (starts_with("\\left\\lfloor")) {
            i += 12;
            size_t end_bar = text.find("\\right\\rfloor", i);
            if (end_bar != std::string_view::npos) {
                std::string inner = std::string(text.substr(i, end_bar - i));
                i = end_bar + 13;
                out += "floor(" + convert_latex_expr(inner) + ")";
                continue;
            }
        }
        if (starts_with("\\left\\lceil")) {
            i += 11;
            size_t end_bar = text.find("\\right\\rceil", i);
            if (end_bar != std::string_view::npos) {
                std::string inner = std::string(text.substr(i, end_bar - i));
                i = end_bar + 12;
                out += "ceil(" + convert_latex_expr(inner) + ")";
                continue;
            }
        }

        // 5. Delimiters: \left(, \right), \left[, \right], \left\{, \right\}
        if (starts_with("\\left(") || starts_with("\\left[") || starts_with("\\left\\{")) {
            i += starts_with("\\left\\{") ? 7 : 6;
            out += "(";
            continue;
        }
        if (starts_with("\\right)") || starts_with("\\right]") || starts_with("\\right\\}")) {
            i += starts_with("\\right\\}") ? 8 : 7;
            out += ")";
            continue;
        }
        if (starts_with("\\left.") || starts_with("\\right.")) {
            i += 6;
            continue;
        }

        // 6. Text / Operators: \operatorname{...}, \text{...}, \mathrm{...}
        if (starts_with("\\operatorname") || starts_with("\\text") || starts_with("\\mathrm")) {
            size_t cmd_len = starts_with("\\operatorname") ? 13 : (starts_with("\\text") ? 5 : 7);
            i += cmd_len;
            std::string content = extract_group(text, i, '{', '}');
            std::string clean_name;
            for (size_t c_idx = 0; c_idx < content.size(); ++c_idx) {
                if (content[c_idx] == '\\' && c_idx + 1 < content.size() && content[c_idx + 1] == '_') {
                    clean_name += '_';
                    ++c_idx;
                } else if (content[c_idx] != '{' && content[c_idx] != '}') {
                    clean_name += content[c_idx];
                }
            }
            skip_spaces(text, i);
            // Check if subscript follows: e.g. \text{d2u}_{dx2} or \text{lap}_{u}
            if (i < n && text[i] == '_') {
                ++i;
                std::string sub = extract_group(text, i, '{', '}');
                std::string clean_sub;
                for (size_t c_idx = 0; c_idx < sub.size(); ++c_idx) {
                    if (sub[c_idx] == '\\' && c_idx + 1 < sub.size() && sub[c_idx + 1] == '_') {
                        clean_sub += '_';
                        ++c_idx;
                    } else if (sub[c_idx] != '{' && sub[c_idx] != '}') {
                        clean_sub += sub[c_idx];
                    }
                }
                clean_name += "_" + clean_sub;
            }
            if (!out.empty() && (std::isalnum(static_cast<unsigned char>(out.back())) || out.back() == ')')) {
                out += " * ";
            }
            out += clean_name;
            continue;
        }

        // 7. Math functions: \sin, \cos, etc. with possible power: \sin^{2}(x)
        bool matched_fn = false;
        const auto& fn_map = get_latex_fn_to_name();
        for (const auto& [cmd, fname] : fn_map) {
            if (starts_with(cmd)) {
                size_t c_end = i + cmd.size();
                if (c_end < n && std::isalpha(static_cast<unsigned char>(text[c_end]))) {
                    continue;
                }
                i += cmd.size();
                matched_fn = true;
                skip_spaces(text, i);

                // Check power: \sin^{2}(x) or \sin^2 x
                std::string power;
                if (i < n && text[i] == '^') {
                    ++i;
                    power = extract_group(text, i, '{', '}');
                    skip_spaces(text, i);
                }

                if (!out.empty() && (std::isalnum(static_cast<unsigned char>(out.back())) || out.back() == ')')) {
                    out += " * ";
                }

                if (power.empty()) {
                    out += fname;
                } else {
                    // Collect argument
                    std::string arg;
                    if (starts_with("\\left(") || (i < n && text[i] == '(')) {
                        bool is_left = starts_with("\\left(");
                        i += is_left ? 6 : 1;
                        int p_depth = 1;
                        size_t arg_start = i;
                        while (i < n && p_depth > 0) {
                            if (starts_with("\\left(")) { i += 6; ++p_depth; }
                            else if (starts_with("\\right)")) { i += 7; --p_depth; }
                            else if (text[i] == '(') { ++i; ++p_depth; }
                            else if (text[i] == ')') { ++i; --p_depth; }
                            else ++i;
                        }
                        size_t arg_end = (p_depth == 0) ? (starts_with("\\right)") ? i - 7 : i - 1) : i;
                        arg = convert_latex_expr(text.substr(arg_start, arg_end - arg_start));
                        std::string c_pow = convert_latex_expr(power);
                        if (is_simple_atom(c_pow)) {
                            out += fname + "(" + arg + ")^" + c_pow;
                        } else {
                            out += fname + "(" + arg + ")^(" + c_pow + ")";
                        }
                    } else {
                        std::string c_pow = convert_latex_expr(power);
                        if (is_simple_atom(c_pow)) {
                            out += fname + "^" + c_pow;
                        } else {
                            out += fname + "^(" + c_pow + ")";
                        }
                    }
                }
                break;
            }
        }
        if (matched_fn) continue;

        // 8. Exponential: e^{...}
        if (text[i] == 'e' && i + 1 < n && text[i+1] == '^') {
            i += 2;
            std::string exp_val = extract_group(text, i, '{', '}');
            if (!out.empty() && (std::isalnum(static_cast<unsigned char>(out.back())) || out.back() == ')')) {
                out += " * ";
            }
            out += "exp(" + convert_latex_expr(exp_val) + ")";
            continue;
        }

        // 9. Greek letters: \alpha, \theta, \nu, \pi, etc.
        bool matched_greek = false;
        const auto& greek_map = get_latex_greek_to_name();
        for (const auto& [cmd, gname] : greek_map) {
            if (starts_with(cmd)) {
                size_t c_end = i + cmd.size();
                if (c_end < n && std::isalpha(static_cast<unsigned char>(text[c_end]))) {
                    continue;
                }
                i += cmd.size();
                matched_greek = true;
                if (!out.empty() && (std::isalnum(static_cast<unsigned char>(out.back())) || out.back() == ')')) {
                    out += " * ";
                }
                std::string var_out = gname;
                skip_spaces(text, i);
                if (i < n && text[i] == '_') {
                    ++i;
                    std::string sub = extract_group(text, i, '{', '}');
                    var_out += "_" + sub;
                }
                out += var_out;
                break;
            }
        }
        if (matched_greek) continue;

        // 10. Math operators & symbols
        if (starts_with("\\cdot") || starts_with("\\times")) {
            i += starts_with("\\cdot") ? 5 : 6;
            out += " * ";
            continue;
        }
        if (starts_with("\\div")) {
            i += 4;
            out += " / ";
            continue;
        }
        if (starts_with("\\pm")) {
            i += 3;
            out += " + ";
            continue;
        }
        if (starts_with("\\bmod")) {
            i += 5;
            out += " % ";
            continue;
        }
        if (starts_with("\\leq") || starts_with("\\le")) {
            i += starts_with("\\leq") ? 4 : 3;
            out += " <= ";
            continue;
        }
        if (starts_with("\\geq") || starts_with("\\ge")) {
            i += starts_with("\\geq") ? 4 : 3;
            out += " >= ";
            continue;
        }
        if (starts_with("\\neq") || starts_with("\\ne")) {
            i += starts_with("\\neq") ? 4 : 3;
            out += " != ";
            continue;
        }
        if (starts_with("\\land")) { i += 5; out += " && "; continue; }
        if (starts_with("\\lor")) { i += 4; out += " || "; continue; }
        if (starts_with("\\neg")) { i += 4; out += "!"; continue; }
        if (starts_with("\\infty") || starts_with("\\inf")) {
            i += starts_with("\\infty") ? 6 : 4;
            out += "inf";
            continue;
        }

        // 11. Ignored spacing commands: \, \; \: \! \quad \qquad
        if (starts_with("\\quad")) { i += 5; out += " "; continue; }
        if (starts_with("\\qquad")) { i += 6; out += " "; continue; }
        if (starts_with("\\,") || starts_with("\\;") || starts_with("\\:") || starts_with("\\!")) {
            i += 2;
            continue;
        }

        // 12. Subscripts on identifiers: e.g. u_{0} -> u0, x_{1} -> x1, u_{k} -> u_k
        if (c == '_') {
            ++i;
            std::string sub = extract_group(text, i, '{', '}');
            bool all_digits = !sub.empty();
            for (char sc : sub) {
                if (!std::isdigit(static_cast<unsigned char>(sc))) {
                    all_digits = false;
                    break;
                }
            }
            if (all_digits && !out.empty() && std::isalpha(static_cast<unsigned char>(out.back()))) {
                out += sub; // u0, x1
            } else {
                out += "_" + sub; // lap_u, d2u_dx2
            }
            continue;
        }

        // 13. Exponents on atoms: ^{...}
        if (c == '^') {
            ++i;
            std::string exp_val = extract_group(text, i, '{', '}');
            std::string c_exp = convert_latex_expr(exp_val);
            if (is_simple_atom(c_exp)) {
                out += "^" + c_exp;
            } else {
                out += "^(" + c_exp + ")";
            }
            continue;
        }

        // 14. Braces: { ... }
        if (c == '{') {
            std::string inner = extract_group(text, i, '{', '}');
            out += "(" + convert_latex_expr(inner) + ")";
            continue;
        }

        // 15. Standard characters and operators
        if (c == '\\') {
            ++i;
            size_t c_start = i;
            while (i < n && std::isalpha(static_cast<unsigned char>(text[i]))) ++i;
            std::string cmd(text.substr(c_start, i - c_start));
            out += cmd;
            continue;
        }

        if (c == ',') {
            out += ", ";
            ++i;
            continue;
        }

        if (c == '+' || c == '=') {
            if (!out.empty() && out.back() != ' ' && out.back() != '(') out += " ";
            out += c;
            out += " ";
            ++i;
            continue;
        }

        if (c == '-') {
            bool is_unary = out.empty();
            if (!is_unary) {
                char last = out.back();
                if (last == ' ' && out.size() >= 2) {
                    char prev = out[out.size() - 2];
                    if (prev == '(' || prev == '=' || prev == '+' || prev == '-' || prev == '*' || prev == '/' || prev == ',') {
                        is_unary = true;
                    }
                } else if (last == '(' || last == '=' || last == '+' || last == '-' || last == '*' || last == '/' || last == ',') {
                    is_unary = true;
                }
            }
            if (is_unary) {
                out += "-";
            } else {
                if (!out.empty() && out.back() != ' ') out += " ";
                out += "- ";
            }
            ++i;
            continue;
        }

        out += c;
        ++i;
    }

    // Clean up double spaces
    std::string clean;
    clean.reserve(out.size());
    for (size_t k = 0; k < out.size(); ++k) {
        if (out[k] == ' ' && !clean.empty() && clean.back() == ' ') continue;
        clean += out[k];
    }
    while (!clean.empty() && clean.back() == ' ') clean.pop_back();
    return clean;
}

} // namespace

Result<std::string> LatexConverter::to_script(std::string_view latex_text) {
    std::string s(latex_text);

    // 1. Strip comments (% ...)
    std::string no_comments;
    no_comments.reserve(s.size());
    bool in_comment = false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\n' || s[i] == '\r') {
            in_comment = false;
            no_comments += s[i];
        } else if (s[i] == '%' && (i == 0 || s[i - 1] != '\\')) {
            in_comment = true;
        } else if (!in_comment) {
            no_comments += s[i];
        }
    }
    s = std::move(no_comments);

    // 2. Trim whitespace
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n' || s.back() == ';')) {
        s.pop_back();
    }
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) {
        s.erase(0, 1);
    }

    if (s.empty()) {
        return std::string("");
    }

    // 3. Strip outer display math delimiters \[ ... \], $$ ... $$, $ ... $
    if (s.starts_with("\\[") && s.ends_with("\\]")) {
        s = s.substr(2, s.size() - 4);
    } else if (s.starts_with("$$") && s.ends_with("$$") && s.size() >= 4) {
        s = s.substr(2, s.size() - 4);
    } else if (s.starts_with("$") && s.ends_with("$") && s.size() >= 2) {
        s = s.substr(1, s.size() - 2);
    }

    // Trim again
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.erase(0, 1);

    // 4. Check for \begin{aligned} / \begin{align*} / etc.
    size_t env_start = s.find("\\begin{");
    size_t env_end = s.rfind("\\end{");
    if (env_start != std::string::npos && env_end != std::string::npos && env_end > env_start) {
        size_t close_brace = s.find('}', env_start);
        if (close_brace != std::string::npos && close_brace < env_end) {
            s = s.substr(close_brace + 1, env_end - (close_brace + 1));
        }
    }

    // 5. Split by \\ for multiline
    std::vector<std::string> raw_lines;
    size_t line_start = 0;
    while (line_start < s.size()) {
        size_t dslash = s.find("\\\\", line_start);
        if (dslash == std::string::npos) {
            raw_lines.push_back(s.substr(line_start));
            break;
        }
        raw_lines.push_back(s.substr(line_start, dslash - line_start));
        line_start = dslash + 2;
        if (line_start < s.size() && s[line_start] == '[') {
            size_t rb = s.find(']', line_start);
            if (rb != std::string::npos) line_start = rb + 1;
        }
    }

    struct ParsedEq {
        std::string lhs;
        std::string rhs;
        bool is_eq{false};
    };
    std::vector<ParsedEq> parsed_eqs;
    for (size_t l_idx = 0; l_idx < raw_lines.size(); ++l_idx) {
        std::string line = raw_lines[l_idx];
        size_t amp_eq = line.find("&=");
        if (amp_eq != std::string::npos) {
            line.replace(amp_eq, 2, "=");
        }
        size_t amp = line.find('&');
        while (amp != std::string::npos) {
            line.replace(amp, 1, " ");
            amp = line.find('&', amp + 1);
        }

        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r' || line.back() == '\n' || line.back() == ';')) line.pop_back();
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t' || line.front() == '\r' || line.front() == '\n')) line.erase(0, 1);
        if (line.empty()) continue;

        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string lhs_raw = line.substr(0, eq_pos);
            std::string rhs_raw = line.substr(eq_pos + 1);
            std::string lhs_conv = convert_latex_expr(lhs_raw);
            std::string rhs_conv = convert_latex_expr(rhs_raw);

            while (!lhs_conv.empty() && lhs_conv.back() == ' ') lhs_conv.pop_back();
            while (!lhs_conv.empty() && lhs_conv.front() == ' ') lhs_conv.erase(0, 1);
            while (!rhs_conv.empty() && rhs_conv.back() == ' ') rhs_conv.pop_back();
            while (!rhs_conv.empty() && rhs_conv.front() == ' ') rhs_conv.erase(0, 1);
            parsed_eqs.push_back({std::move(lhs_conv), std::move(rhs_conv), true});
        } else {
            std::string conv = convert_latex_expr(line);
            while (!conv.empty() && conv.back() == ' ') conv.pop_back();
            while (!conv.empty() && conv.front() == ' ') conv.erase(0, 1);
            parsed_eqs.push_back({"", std::move(conv), false});
        }
    }

    // Check for 2D parametric / polar system: { x = ... , y = ... }
    if (parsed_eqs.size() == 2 && parsed_eqs[0].is_eq && parsed_eqs[1].is_eq) {
        std::string rhs_x, rhs_y;
        bool is_xy_system = false;
        if (parsed_eqs[0].lhs == "x" && parsed_eqs[1].lhs == "y") {
            rhs_x = parsed_eqs[0].rhs;
            rhs_y = parsed_eqs[1].rhs;
            is_xy_system = true;
        } else if (parsed_eqs[0].lhs == "y" && parsed_eqs[1].lhs == "x") {
            rhs_x = parsed_eqs[1].rhs;
            rhs_y = parsed_eqs[0].rhs;
            is_xy_system = true;
        }

        if (is_xy_system) {
            std::vector<std::string> angles = {"theta", "t", "phi"};
            std::string radial_expr;
            std::string angle_name;
            bool is_polar = false;

            for (const auto& ang : angles) {
                std::string c_suff = "* cos(" + ang + ")";
                std::string s_suff = "* sin(" + ang + ")";
                if (rhs_x.size() > c_suff.size() && rhs_y.size() > s_suff.size()) {
                    if (rhs_x.ends_with(c_suff) && rhs_y.ends_with(s_suff)) {
                        std::string rx = rhs_x.substr(0, rhs_x.size() - c_suff.size());
                        std::string ry = rhs_y.substr(0, rhs_y.size() - s_suff.size());
                        while (!rx.empty() && (rx.back() == ' ' || rx.back() == '*')) rx.pop_back();
                        while (!ry.empty() && (ry.back() == ' ' || ry.back() == '*')) ry.pop_back();
                        if (!rx.empty() && rx == ry) {
                            radial_expr = rx;
                            angle_name = ang;
                            is_polar = true;
                            break;
                        }
                    }
                }
            }

            auto find_free_params = [](std::string_view expr, const std::unordered_set<std::string>& excluded) -> std::vector<std::string> {
                std::vector<std::string> res;
                std::unordered_set<std::string> seen;
                size_t idx = 0;
                while (idx < expr.size()) {
                    if (std::isalpha(static_cast<unsigned char>(expr[idx])) || expr[idx] == '_') {
                        size_t st = idx;
                        while (idx < expr.size() && (std::isalnum(static_cast<unsigned char>(expr[idx])) || expr[idx] == '_')) ++idx;
                        std::string id(expr.substr(st, idx - st));
                        if (excluded.count(id)) continue;
                        static const std::unordered_set<std::string> kBuiltins = {
                            "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
                            "sinh", "cosh", "tanh", "exp", "ln", "log", "sqrt", "cbrt", "abs",
                            "hypot", "min", "max", "clamp", "floor", "ceil", "round",
                            "pi", "e", "tau", "phi", "euler", "inf"
                        };
                        if (kBuiltins.count(id)) continue;
                        if (seen.insert(id).second) {
                            res.push_back(id);
                        }
                    } else {
                        ++idx;
                    }
                }
                return res;
            };

            if (is_polar) {
                std::string script;
                auto params = find_free_params(radial_expr, {angle_name, "r", "x", "y"});
                for (const auto& p : params) {
                    script += "let " + p + " = 3.0;\n";
                }
                script += "let r = hypot(x, y);\n";
                script += "let " + angle_name + " = atan2(y, x);\n";
                script += "r - " + radial_expr + " = 0";
                return script;
            } else {
                std::string script;
                auto params = find_free_params(rhs_x + " " + rhs_y, {"t", "theta", "phi", "x", "y"});
                for (const auto& p : params) {
                    script += "let " + p + " = 1.0;\n";
                }
                if (rhs_x.find("theta") != std::string::npos || rhs_y.find("theta") != std::string::npos) {
                    script += "let theta = t;\n";
                }
                script += "let x = " + rhs_x + ";\n";
                script += "let y = " + rhs_y + ";";
                return script;
            }
        }
    }

    std::string result_script;
    for (size_t l_idx = 0; l_idx < parsed_eqs.size(); ++l_idx) {
        const auto& item = parsed_eqs[l_idx];
        if (item.is_eq) {
            const std::string& lhs_conv = item.lhs;
            const std::string& rhs_conv = item.rhs;

            if (lhs_conv == "f(x, y)" || lhs_conv == "f(x)") {
                result_script += rhs_conv + ";\n";
                continue;
            }

            bool is_simple_var = is_simple_atom(lhs_conv);
            if (is_simple_var && parsed_eqs.size() > 1 && l_idx + 1 < parsed_eqs.size()) {
                result_script += "let " + lhs_conv + " = " + rhs_conv + ";\n";
            } else if (is_simple_var && parsed_eqs.size() > 1 && l_idx + 1 == parsed_eqs.size()) {
                result_script += "let " + lhs_conv + " = " + rhs_conv + ";\n";
            } else if (parsed_eqs.size() > 1 && !is_simple_var) {
                result_script += lhs_conv + " = " + rhs_conv + "\n";
            } else {
                if (lhs_conv == "y" && parsed_eqs.size() == 1) {
                    result_script += rhs_conv;
                } else {
                    result_script += lhs_conv + " = " + rhs_conv;
                }
            }
        } else {
            if (parsed_eqs.size() > 1) {
                result_script += item.rhs + ";\n";
            } else {
                result_script += item.rhs;
            }
        }
    }

    while (!result_script.empty() && (result_script.back() == '\n' || result_script.back() == '\r')) {
        result_script.pop_back();
    }

    return result_script;
}

} // namespace formulaic
