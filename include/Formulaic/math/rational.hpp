#pragma once

#include <Formulaic/core/export.hpp>
#include <Formulaic/math/bigint.hpp>
#include <compare>
#include <iosfwd>
#include <string>
#include <string_view>

namespace formulaic::math {

class FORMULAIC_API Rational {
public:
    Rational();
    Rational(int64_t num, int64_t den = 1);
    Rational(const BigInt& num, const BigInt& den = BigInt(1));
    explicit Rational(std::string_view str, int base = 10);
    explicit Rational(const char* str, int base = 10);
    explicit Rational(const std::string& str, int base = 10);

    Rational(const Rational& other);
    Rational(Rational&& other) noexcept;
    Rational& operator=(const Rational& other);
    Rational& operator=(Rational&& other) noexcept;
    ~Rational();

    // Arithmetic operators
    Rational operator+(const Rational& other) const;
    Rational operator-(const Rational& other) const;
    Rational operator*(const Rational& other) const;
    Rational operator/(const Rational& other) const;

    Rational& operator+=(const Rational& other);
    Rational& operator-=(const Rational& other);
    Rational& operator*=(const Rational& other);
    Rational& operator/=(const Rational& other);

    Rational operator-() const;
    Rational operator+() const;

    // Comparisons
    bool operator==(const Rational& other) const noexcept;
    bool operator!=(const Rational& other) const noexcept;
    bool operator<(const Rational& other) const noexcept;
    bool operator<=(const Rational& other) const noexcept;
    bool operator>(const Rational& other) const noexcept;
    bool operator>=(const Rational& other) const noexcept;
    std::strong_ordering operator<=>(const Rational& other) const noexcept;

    // Queries & Conversions
    [[nodiscard]] BigInt num() const;
    [[nodiscard]] BigInt den() const;
    [[nodiscard]] std::string to_string(int base = 10) const;
    [[nodiscard]] double to_double() const noexcept;
    [[nodiscard]] bool is_zero() const noexcept;
    [[nodiscard]] bool is_negative() const noexcept;
    [[nodiscard]] bool is_positive() const noexcept;
    [[nodiscard]] int sign() const noexcept;
    [[nodiscard]] Rational abs() const;
    [[nodiscard]] Rational inv() const;

    // Low-level GMP handle access
#if defined(FORMULAIC_HAS_GMP)
    [[nodiscard]] mpq_ptr raw() noexcept { return val_; }
    [[nodiscard]] mpq_srcptr raw() const noexcept { return val_; }
#endif

    friend FORMULAIC_API std::ostream& operator<<(std::ostream& os, const Rational& rat);

private:
    void canonicalize();

#if defined(FORMULAIC_HAS_GMP)
    mpq_t val_;
#else
    int64_t fallback_num_{0};
    int64_t fallback_den_{1};
#endif
};

} // namespace formulaic::math
