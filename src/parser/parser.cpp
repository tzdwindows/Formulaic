#include "parser.hpp"
#include <cmath>
#include <unordered_map>

namespace formulaic {

namespace {

const std::unordered_map<std::string_view, double> kBuiltinConstants = {
    {"pi", 3.14159265358979323846},
    {"PI", 3.14159265358979323846},
    {"e", 2.71828182845904523536},
    {"E", 2.71828182845904523536},
    {"tau", 6.28318530717958647692},
    {"TAU", 6.28318530717958647692},
    {"phi", 1.61803398874989484820},
    {"PHI", 1.61803398874989484820}
};

const std::unordered_map<std::string_view, size_t> kKnownFunctions = {
    {"sin", 1}, {"cos", 1}, {"tan", 1},
    {"asin", 1}, {"acos", 1}, {"atan", 1},
    {"sinh", 1}, {"cosh", 1}, {"tanh", 1},
    {"exp", 1}, {"ln", 1}, {"log", 1}, {"log10", 1}, {"log2", 1},
    {"sqrt", 1}, {"cbrt", 1}, {"abs", 1},
    {"floor", 1}, {"ceil", 1}, {"round", 1}, {"sign", 1},
    {"atan2", 2}, {"min", 2}, {"max", 2}, {"pow", 2},
    {"clamp", 3}
};

} // anonymous namespace

Parser::Parser(std::vector<Token> tokens, std::vector<std::string> variable_names)
    : tokens_(std::move(tokens)) {
    for (auto& var : variable_names) {
        known_variables_.insert(std::move(var));
    }
}

const Token& Parser::peek() const noexcept {
    return tokens_[current_];
}

const Token& Parser::previous() const noexcept {
    return tokens_[current_ - 1];
}

bool Parser::is_at_end() const noexcept {
    return peek().type == TokenType::EndOfFile;
}

const Token& Parser::advance() noexcept {
    if (!is_at_end()) current_++;
    return previous();
}

bool Parser::check(TokenType type) const noexcept {
    if (is_at_end()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) noexcept {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Result<std::unique_ptr<ASTNode>> Parser::parse() {
    if (tokens_.empty() || tokens_[0].type == TokenType::EndOfFile) {
        return Diagnostic{
            ErrorCode::UnexpectedToken,
            "Empty expression",
            SourceLocation{1, 1, 0}
        };
    }

    auto expr = parse_expression();
    if (!expr) return expr;

    if (!is_at_end()) {
        const auto& tok = peek();
        return Diagnostic{
            ErrorCode::UnexpectedToken,
            "Unexpected trailing token '" + std::string(tok.text) + "' after valid expression",
            tok.location
        };
    }

    return expr;
}

Result<std::unique_ptr<ASTNode>> Parser::parse_expression() {
    return parse_logical_or();
}

Result<std::unique_ptr<ASTNode>> Parser::parse_logical_or() {
    auto left_res = parse_logical_and();
    if (!left_res) return left_res;
    auto left = std::move(left_res.value());

    while (match(TokenType::PipePipe)) {
        SourceLocation loc = previous().location;
        auto right_res = parse_logical_and();
        if (!right_res) return right_res;
        left = std::make_unique<BinaryOpNode>(BinaryOp::LogicalOr, std::move(left), std::move(right_res.value()), loc);
    }
    return left;
}

Result<std::unique_ptr<ASTNode>> Parser::parse_logical_and() {
    auto left_res = parse_equality();
    if (!left_res) return left_res;
    auto left = std::move(left_res.value());

    while (match(TokenType::AmpAmp)) {
        SourceLocation loc = previous().location;
        auto right_res = parse_equality();
        if (!right_res) return right_res;
        left = std::make_unique<BinaryOpNode>(BinaryOp::LogicalAnd, std::move(left), std::move(right_res.value()), loc);
    }
    return left;
}

Result<std::unique_ptr<ASTNode>> Parser::parse_equality() {
    auto left_res = parse_relational();
    if (!left_res) return left_res;
    auto left = std::move(left_res.value());

    while (check(TokenType::EqualEqual) || check(TokenType::ExclamationEqual)) {
        Token op_tok = advance();
        BinaryOp op = (op_tok.type == TokenType::EqualEqual) ? BinaryOp::Equal : BinaryOp::NotEqual;
        auto right_res = parse_relational();
        if (!right_res) return right_res;
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right_res.value()), op_tok.location);
    }
    return left;
}

Result<std::unique_ptr<ASTNode>> Parser::parse_relational() {
    auto left_res = parse_additive();
    if (!left_res) return left_res;
    auto left = std::move(left_res.value());

    while (check(TokenType::Less) || check(TokenType::LessEqual) ||
           check(TokenType::Greater) || check(TokenType::GreaterEqual)) {
        Token op_tok = advance();
        BinaryOp op = BinaryOp::Less;
        if (op_tok.type == TokenType::LessEqual) op = BinaryOp::LessEqual;
        else if (op_tok.type == TokenType::Greater) op = BinaryOp::Greater;
        else if (op_tok.type == TokenType::GreaterEqual) op = BinaryOp::GreaterEqual;

        auto right_res = parse_additive();
        if (!right_res) return right_res;
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right_res.value()), op_tok.location);
    }
    return left;
}

Result<std::unique_ptr<ASTNode>> Parser::parse_additive() {
    auto left_res = parse_multiplicative();
    if (!left_res) return left_res;
    auto left = std::move(left_res.value());

    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        Token op_tok = advance();
        BinaryOp op = (op_tok.type == TokenType::Plus) ? BinaryOp::Add : BinaryOp::Subtract;
        auto right_res = parse_multiplicative();
        if (!right_res) return right_res;
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right_res.value()), op_tok.location);
    }
    return left;
}

bool Parser::can_start_implicit_multiplication() const noexcept {
    if (is_at_end()) return false;
    TokenType t = peek().type;
    return (t == TokenType::Number || t == TokenType::Identifier || t == TokenType::LParen);
}

Result<std::unique_ptr<ASTNode>> Parser::parse_multiplicative() {
    auto left_res = parse_unary();
    if (!left_res) return left_res;
    auto left = std::move(left_res.value());

    while (true) {
        if (check(TokenType::Star) || check(TokenType::Slash) || check(TokenType::Percent)) {
            Token op_tok = advance();
            BinaryOp op = BinaryOp::Multiply;
            if (op_tok.type == TokenType::Slash) op = BinaryOp::Divide;
            else if (op_tok.type == TokenType::Percent) op = BinaryOp::Modulo;

            auto right_res = parse_unary();
            if (!right_res) return right_res;
            left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right_res.value()), op_tok.location);
        } else if (can_start_implicit_multiplication()) {
            // Implicit multiplication: 2x, 3(x+1), x y
            SourceLocation loc = peek().location;
            auto right_res = parse_unary();
            if (!right_res) return right_res;
            left = std::make_unique<BinaryOpNode>(BinaryOp::Multiply, std::move(left), std::move(right_res.value()), loc);
        } else {
            break;
        }
    }
    return left;
}

Result<std::unique_ptr<ASTNode>> Parser::parse_unary() {
    if (match(TokenType::Plus)) {
        return parse_unary(); // unary positive identity
    }
    if (match(TokenType::Minus)) {
        SourceLocation loc = previous().location;
        auto operand_res = parse_unary();
        if (!operand_res) return operand_res;
        return std::make_unique<UnaryOpNode>(UnaryOp::Negate, std::move(operand_res.value()), loc);
    }
    if (match(TokenType::Exclamation)) {
        SourceLocation loc = previous().location;
        auto operand_res = parse_unary();
        if (!operand_res) return operand_res;
        return std::make_unique<UnaryOpNode>(UnaryOp::Not, std::move(operand_res.value()), loc);
    }
    return parse_power();
}

Result<std::unique_ptr<ASTNode>> Parser::parse_power() {
    auto base_res = parse_primary();
    if (!base_res) return base_res;
    auto base = std::move(base_res.value());

    if (match(TokenType::Caret)) {
        SourceLocation loc = previous().location;
        // Exponentiation is right-associative: 2^3^2 = 2^(3^2)
        auto exp_res = parse_unary();
        if (!exp_res) return exp_res;
        return std::make_unique<BinaryOpNode>(BinaryOp::Power, std::move(base), std::move(exp_res.value()), loc);
    }
    return base;
}

Result<std::unique_ptr<ASTNode>> Parser::parse_primary() {
    if (match(TokenType::Number)) {
        const auto& tok = previous();
        return std::make_unique<NumberNode>(tok.number_value, tok.location);
    }

    if (match(TokenType::LParen)) {
        SourceLocation loc = previous().location;
        auto expr_res = parse_expression();
        if (!expr_res) return expr_res;

        if (!match(TokenType::RParen)) {
            return Diagnostic{
                ErrorCode::MissingParenthesis,
                "Expected closing ')' matching opening '(' at line " +
                    std::to_string(loc.line) + ", column " + std::to_string(loc.column),
                peek().location
            };
        }
        return expr_res;
    }

    if (match(TokenType::Identifier)) {
        const auto& tok = previous();
        std::string name(tok.text);

        // Check if followed by '(' -> Function call
        if (check(TokenType::LParen)) {
            advance(); // consume '('
            std::vector<std::unique_ptr<ASTNode>> args;

            if (!check(TokenType::RParen)) {
                do {
                    auto arg = parse_expression();
                    if (!arg) return arg;
                    args.push_back(std::move(arg.value()));
                } while (match(TokenType::Comma));
            }

            if (!match(TokenType::RParen)) {
                return Diagnostic{
                    ErrorCode::MissingParenthesis,
                    "Expected ')' closing function arguments for '" + name + "'",
                    peek().location
                };
            }

            auto fn_it = kKnownFunctions.find(name);
            if (fn_it == kKnownFunctions.end()) {
                return Diagnostic{
                    ErrorCode::UnknownIdentifier,
                    "Unknown function: '" + name + "'",
                    tok.location
                };
            }

            if (args.size() != fn_it->second) {
                return Diagnostic{
                    ErrorCode::InvalidArity,
                    "Function '" + name + "' expects " + std::to_string(fn_it->second) +
                        " argument(s), but " + std::to_string(args.size()) + " provided",
                    tok.location
                };
            }

            return std::make_unique<FunctionCallNode>(std::move(name), std::move(args), tok.location);
        }

        // Constant replacement (e.g. pi, e, tau, phi)
        auto const_it = kBuiltinConstants.find(name);
        if (const_it != kBuiltinConstants.end()) {
            return std::make_unique<NumberNode>(const_it->second, tok.location);
        }

        // Known variable (e.g. x, y, t)
        if (known_variables_.find(name) != known_variables_.end()) {
            return std::make_unique<VariableNode>(std::move(name), tok.location);
        }

        return Diagnostic{
            ErrorCode::UnknownIdentifier,
            "Unknown identifier: '" + name + "'. Not a recognized variable or constant.",
            tok.location
        };
    }

    const auto& tok = peek();
    return Diagnostic{
        ErrorCode::MissingOperand,
        "Expected number, variable, function call, or '(' but found '" + std::string(tok.text) + "'",
        tok.location
    };
}

} // namespace formulaic
