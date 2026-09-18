#include "lexer.hpp"
#include <cctype>
#include <charconv>

namespace formulaic {

Lexer::Lexer(std::string_view source) : source_(source) {}

bool Lexer::is_at_end() const noexcept {
    return current_ >= source_.size();
}

char Lexer::peek() const noexcept {
    if (is_at_end()) return '\0';
    return source_[current_];
}

char Lexer::peek_next() const noexcept {
    if (current_ + 1 >= source_.size()) return '\0';
    return source_[current_ + 1];
}

char Lexer::advance() noexcept {
    return source_[current_++];
}

bool Lexer::match(char expected) noexcept {
    if (is_at_end() || source_[current_] != expected) return false;
    current_++;
    return true;
}

void Lexer::skip_whitespace() noexcept {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            line_++;
            advance();
            line_start_offset_ = current_;
        } else {
            break;
        }
    }
}

Result<std::vector<Token>> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skip_whitespace();
        if (is_at_end()) break;

        start_ = current_;
        const size_t col = start_ - line_start_offset_ + 1;
        const SourceLocation loc{line_, col, start_};
        char c = advance();

        switch (c) {
            case '+': tokens.push_back({TokenType::Plus, "+", 0.0, loc}); break;
            case '-': tokens.push_back({TokenType::Minus, "-", 0.0, loc}); break;
            case '*': tokens.push_back({TokenType::Star, "*", 0.0, loc}); break;
            case '/': tokens.push_back({TokenType::Slash, "/", 0.0, loc}); break;
            case '^': tokens.push_back({TokenType::Caret, "^", 0.0, loc}); break;
            case '%': tokens.push_back({TokenType::Percent, "%", 0.0, loc}); break;
            case '(': tokens.push_back({TokenType::LParen, "(", 0.0, loc}); break;
            case ')': tokens.push_back({TokenType::RParen, ")", 0.0, loc}); break;
            case ',': tokens.push_back({TokenType::Comma, ",", 0.0, loc}); break;

            case ';': tokens.push_back({TokenType::Semicolon, ";", 0.0, loc}); break;

            case '=':
                if (match('=')) {
                    tokens.push_back({TokenType::EqualEqual, "==", 0.0, loc});
                } else {
                    tokens.push_back({TokenType::Equal, "=", 0.0, loc});
                }
                break;

            case '!':
                if (match('=')) {
                    tokens.push_back({TokenType::ExclamationEqual, "!=", 0.0, loc});
                } else {
                    tokens.push_back({TokenType::Exclamation, "!", 0.0, loc});
                }
                break;

            case '<':
                if (match('=')) {
                    tokens.push_back({TokenType::LessEqual, "<=", 0.0, loc});
                } else {
                    tokens.push_back({TokenType::Less, "<", 0.0, loc});
                }
                break;

            case '>':
                if (match('=')) {
                    tokens.push_back({TokenType::GreaterEqual, ">=", 0.0, loc});
                } else {
                    tokens.push_back({TokenType::Greater, ">", 0.0, loc});
                }
                break;

            case '&':
                if (match('&')) {
                    tokens.push_back({TokenType::AmpAmp, "&&", 0.0, loc});
                } else {
                    return Diagnostic{
                        ErrorCode::UnexpectedCharacter,
                        "Single '&' is not supported. Use '&&' for logical AND.",
                        loc
                    };
                }
                break;

            case '|':
                if (match('|')) {
                    tokens.push_back({TokenType::PipePipe, "||", 0.0, loc});
                } else {
                    return Diagnostic{
                        ErrorCode::UnexpectedCharacter,
                        "Single '|' is not supported. Use '||' for logical OR.",
                        loc
                    };
                }
                break;

            default:
                if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && std::isdigit(static_cast<unsigned char>(peek())))) {
                    current_ = start_; // Backtrack one char to scan full number
                    auto res = scan_number();
                    if (!res) return res.error();
                    tokens.push_back(res.value());
                } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                    current_ = start_;
                    auto res = scan_identifier();
                    if (!res) return res.error();
                    tokens.push_back(res.value());
                } else {
                    return Diagnostic{
                        ErrorCode::UnexpectedCharacter,
                        std::string("Unexpected character: '") + c + "'",
                        loc
                    };
                }
                break;
        }
    }

    const size_t col = current_ - line_start_offset_ + 1;
    tokens.push_back({TokenType::EndOfFile, "", 0.0, {line_, col, current_}});
    return tokens;
}

Result<Token> Lexer::scan_number() {
    start_ = current_;
    const size_t col = start_ - line_start_offset_ + 1;
    const SourceLocation loc{line_, col, start_};

    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek_next()))) {
        advance(); // consume '.'
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
    }

    // Scientific notation e.g. 1e5, 1.2e-3
    if (peek() == 'e' || peek() == 'E') {
        advance(); // consume 'e'/'E'
        if (peek() == '+' || peek() == '-') {
            advance();
        }
        if (!std::isdigit(static_cast<unsigned char>(peek()))) {
            return Diagnostic{
                ErrorCode::UnexpectedCharacter,
                "Expected exponent digits in scientific notation",
                loc
            };
        }
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
    }

    std::string_view text = source_.substr(start_, current_ - start_);
    double value = 0.0;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc()) {
        return Diagnostic{
            ErrorCode::UnexpectedCharacter,
            "Failed to parse numeric constant: " + std::string(text),
            loc
        };
    }

    return Token{TokenType::Number, text, value, loc};
}

Result<Token> Lexer::scan_identifier() {
    start_ = current_;
    const size_t col = start_ - line_start_offset_ + 1;
    const SourceLocation loc{line_, col, start_};

    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
        advance();
    }

    std::string_view text = source_.substr(start_, current_ - start_);
    if (text == "let") {
        return Token{TokenType::KeywordLet, text, 0.0, loc};
    }
    if (text == "var") {
        return Token{TokenType::KeywordVar, text, 0.0, loc};
    }
    return Token{TokenType::Identifier, text, 0.0, loc};
}

} // namespace formulaic
