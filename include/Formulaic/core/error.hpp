#pragma once

#include <Formulaic/core/export.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace formulaic {

struct SourceLocation {
    size_t line{1};
    size_t column{1};
    size_t offset{0};
};

enum class ErrorCode {
    Ok = 0,
    UnexpectedCharacter,
    UnexpectedToken,
    UnterminatedString,
    MissingOperand,
    MissingParenthesis,
    UnknownIdentifier,
    InvalidArity,
    DivisionByZero,
    EvaluationError,
    InternalCompilerError
};

struct Diagnostic {
    ErrorCode code{ErrorCode::Ok};
    std::string message;
    SourceLocation location;

    [[nodiscard]] bool ok() const noexcept { return code == ErrorCode::Ok; }

    [[nodiscard]] std::string format() const {
        if (ok()) return "No error";
        return "Error at (" + std::to_string(location.line) + ":" +
               std::to_string(location.column) + "): " + message;
    }
};

template <typename T>
class Result {
public:
    Result(const T& val) : data_(val) {}
    Result(T&& val) : data_(std::move(val)) {}

    template <typename U>
        requires (!std::is_same_v<std::decay_t<U>, Result<T>> &&
                  !std::is_same_v<std::decay_t<U>, Diagnostic> &&
                  std::is_convertible_v<U, T>)
    Result(U&& val) : data_(T(std::forward<U>(val))) {}

    Result(const Diagnostic& diag) : data_(diag) {}
    Result(Diagnostic&& diag) : data_(std::move(diag)) {}

    [[nodiscard]] bool has_value() const noexcept {
        return std::holds_alternative<T>(data_);
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] const T& value() const & {
        return std::get<T>(data_);
    }

    [[nodiscard]] T& value() & {
        return std::get<T>(data_);
    }

    [[nodiscard]] T&& value() && {
        return std::get<T>(std::move(data_));
    }

    [[nodiscard]] const T& operator*() const & { return value(); }
    [[nodiscard]] T& operator*() & { return value(); }
    [[nodiscard]] const T* operator->() const { return &value(); }
    [[nodiscard]] T* operator->() { return &value(); }

    [[nodiscard]] const Diagnostic& error() const {
        return std::get<Diagnostic>(data_);
    }

private:
    std::variant<T, Diagnostic> data_;
};

template <>
class Result<void> {
public:
    Result() : has_error_(false) {}
    Result(const Diagnostic& diag) : diag_(diag), has_error_(true) {}
    Result(Diagnostic&& diag) : diag_(std::move(diag)), has_error_(true) {}

    [[nodiscard]] bool has_value() const noexcept {
        return !has_error_;
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    void value() const noexcept {}

    [[nodiscard]] const Diagnostic& error() const {
        return diag_;
    }

private:
    Diagnostic diag_;
    bool has_error_{false};
};

} // namespace formulaic
