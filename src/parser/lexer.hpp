#pragma once

#include <Formulaic/core/error.hpp>
#include <Formulaic/parser/token.hpp>
#include <string_view>
#include <vector>

namespace formulaic {

class Lexer {
public:
    explicit Lexer(std::string_view source);

    Result<std::vector<Token>> tokenize();

private:
    [[nodiscard]] bool is_at_end() const noexcept;
    [[nodiscard]] char peek() const noexcept;
    [[nodiscard]] char peek_next() const noexcept;
    char advance() noexcept;
    bool match(char expected) noexcept;
    void skip_whitespace() noexcept;

    Result<Token> scan_number();
    Result<Token> scan_identifier();

    std::string_view source_;
    size_t start_{0};
    size_t current_{0};
    size_t line_{1};
    size_t line_start_offset_{0};
};

} // namespace formulaic
