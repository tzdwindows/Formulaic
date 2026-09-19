#pragma once

#include <Formulaic/core/export.hpp>
#include <Formulaic/math/gmp_types.hpp>
#include <compare>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>

namespace formulaic::math {

class FORMULAIC_API BigInt {
public:
    BigInt();
    BigInt(int64_t val);
    BigInt(uint64_t val);
    BigInt(int val);
    BigInt(unsigned int val);
    explicit BigInt(std::string_view str, int base = 10);
    explicit BigInt(const char* str, int base = 10);
    explicit BigInt(const std::string& str, int base = 10);

    BigInt(const BigInt& other);
    BigInt(BigInt&& other) noexcept;
    BigInt& operator=(const BigInt& other);
    BigInt& operator=(BigInt&& other) noexcept;
    ~BigInt();

    // Arithmetic operators
    BigInt operator+(const BigInt& other) const;
    BigInt operator-(const BigInt& other) const;
    BigInt operator*(const BigInt& other) const;
    BigInt operator/(const BigInt& other) const;
    BigInt operator%(const BigInt& other) const;

    BigInt& operator+=(const BigInt& other);
    BigInt& operator-=(const BigInt& other);
    BigInt& operator*=(const BigInt& other);
    BigInt& operator/=(const BigInt& other);
    BigInt& operator%=(const BigInt& other);

    BigInt operator-() const;
    BigInt operator+() const;
    BigInt& operator++();
    BigInt operator++(int);
    BigInt& operator--();
    BigInt operator--(int);

    // Bitwise operators
    BigInt operator&(const BigInt& other) const;
    BigInt operator|(const BigInt& other) const;
    BigInt operator^(const BigInt& other) const;
    BigInt operator~() const;
    BigInt operator<<(unsigned long shift) const;
    BigInt operator>>(unsigned long shift) const;

    BigInt& operator&=(const BigInt& other);
    BigInt& operator|=(const BigInt& other);
    BigInt& operator^=(const BigInt& other);
    BigInt& operator<<=(unsigned long shift);
    BigInt& operator>>=(unsigned long shift);

    // Comparisons
    bool operator==(const BigInt& other) const noexcept;
    bool operator!=(const BigInt& other) const noexcept;
    bool operator<(const BigInt& other) const noexcept;
    bool operator<=(const BigInt& other) const noexcept;
    bool operator>(const BigInt& other) const noexcept;
    bool operator>=(const BigInt& other) const noexcept;
    std::strong_ordering operator<=>(const BigInt& other) const noexcept;

    // Queries & Conversions
    [[nodiscard]] std::string to_string(int base = 10) const;
    [[nodiscard]] int64_t to_int64() const noexcept;
    [[nodiscard]] double to_double() const noexcept;
    [[nodiscard]] bool is_zero() const noexcept;
    [[nodiscard]] bool is_negative() const noexcept;
    [[nodiscard]] bool is_positive() const noexcept;
    [[nodiscard]] bool is_even() const noexcept;
    [[nodiscard]] bool is_odd() const noexcept;
    [[nodiscard]] int sign() const noexcept;
    [[nodiscard]] size_t bit_length() const noexcept;
    [[nodiscard]] BigInt abs() const;

    // High-performance number theoretic functions
    [[nodiscard]] static BigInt gcd(const BigInt& a, const BigInt& b);
    [[nodiscard]] static BigInt lcm(const BigInt& a, const BigInt& b);
    [[nodiscard]] static BigInt factorial(unsigned long n);
    [[nodiscard]] static BigInt fibonacci(unsigned long n);
    [[nodiscard]] static BigInt binomial(unsigned long n, unsigned long k);
    [[nodiscard]] static BigInt pow(const BigInt& base, unsigned long exp);
    [[nodiscard]] static BigInt pow_mod(const BigInt& base, const BigInt& exp, const BigInt& mod);
    [[nodiscard]] static BigInt sqrt(const BigInt& n);
    [[nodiscard]] bool is_probab_prime(int reps = 25) const;
    [[nodiscard]] BigInt next_prime() const;

    // Low-level GMP handle access
#if defined(FORMULAIC_HAS_GMP)
    [[nodiscard]] mpz_ptr raw() noexcept { return val_; }
    [[nodiscard]] mpz_srcptr raw() const noexcept { return val_; }
#endif

    friend FORMULAIC_API std::ostream& operator<<(std::ostream& os, const BigInt& bi);

private:
#if defined(FORMULAIC_HAS_GMP)
    mpz_t val_;
#else
    int64_t fallback_val_{0};
#endif
};

} // namespace formulaic::math
