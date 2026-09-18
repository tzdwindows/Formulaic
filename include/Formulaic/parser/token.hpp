#pragma once

#include <Formulaic/core/error.hpp>
#include <string>
#include <string_view>

namespace formulaic {

enum class TokenType {
    EndOfFile = 0,
    Number,
    Identifier,
    
    // Arithmetic operators
    Plus,           // +
    Minus,          // -
    Star,           // *
    Slash,          // /
    Caret,          // ^ (power)
    Percent,        // % (modulo)
    
    // Comparison operators
    EqualEqual,     // ==
    ExclamationEqual,// !=
    Less,           // <
    LessEqual,      // <=
    Greater,        // >
    GreaterEqual,   // >=

    // Logical operators
    AmpAmp,         // &&
    PipePipe,       // ||
    Exclamation,    // !
    
    // Delimiters
    LParen,         // (
    RParen,         // )
    Comma,          // ,

    Invalid
};

struct Token {
    TokenType type{TokenType::Invalid};
    std::string_view text;
    double number_value{0.0};
    SourceLocation location;
};

[[nodiscard]] constexpr std::string_view token_type_name(TokenType type) noexcept {
    switch (type) {
        case TokenType::EndOfFile: return "EndOfFile";
        case TokenType::Number: return "Number";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Star: return "*";
        case TokenType::Slash: return "/";
        case TokenType::Caret: return "^";
        case TokenType::Percent: return "%";
        case TokenType::EqualEqual: return "==";
        case TokenType::ExclamationEqual: return "!=";
        case TokenType::Less: return "<";
        case TokenType::LessEqual: return "<=";
        case TokenType::Greater: return ">";
        case TokenType::GreaterEqual: return ">=";
        case TokenType::AmpAmp: return "&&";
        case TokenType::PipePipe: return "||";
        case TokenType::Exclamation: return "!";
        case TokenType::LParen: return "(";
        case TokenType::RParen: return ")";
        case TokenType::Comma: return ",";
        case TokenType::Invalid: return "Invalid";
        default: return "Unknown";
    }
}

} // namespace formulaic
