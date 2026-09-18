#pragma once

#include <Formulaic/core/error.hpp>
#include <Formulaic/parser/ast.hpp>
#include <Formulaic/parser/token.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace formulaic {

class Parser {
public:
    Parser(std::vector<Token> tokens, std::vector<std::string> variable_names);

    Result<std::unique_ptr<ASTNode>> parse();

private:
    [[nodiscard]] const Token& peek() const noexcept;
    [[nodiscard]] const Token& previous() const noexcept;
    [[nodiscard]] bool is_at_end() const noexcept;
    const Token& advance() noexcept;
    bool check(TokenType type) const noexcept;
    bool match(TokenType type) noexcept;

    Result<std::unique_ptr<ASTNode>> parse_statement();
    Result<std::unique_ptr<ASTNode>> parse_expression();
    Result<std::unique_ptr<ASTNode>> parse_logical_or();
    Result<std::unique_ptr<ASTNode>> parse_logical_and();
    Result<std::unique_ptr<ASTNode>> parse_equality();
    Result<std::unique_ptr<ASTNode>> parse_relational();
    Result<std::unique_ptr<ASTNode>> parse_additive();
    Result<std::unique_ptr<ASTNode>> parse_multiplicative();
    Result<std::unique_ptr<ASTNode>> parse_unary();
    Result<std::unique_ptr<ASTNode>> parse_power();
    Result<std::unique_ptr<ASTNode>> parse_primary();

    [[nodiscard]] bool can_start_implicit_multiplication() const noexcept;

    std::vector<Token> tokens_;
    size_t current_{0};
    std::unordered_set<std::string> known_variables_;
};

} // namespace formulaic
