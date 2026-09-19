#include <Formulaic/math/bigint.hpp>
#include <Formulaic/math/gmp_types.hpp>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

namespace formulaic::math {

const char* get_gmp_backend_info() noexcept {
    return FORMULAIC_GMP_BACKEND_NAME;
}

bool has_gmp_support() noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return true;
#else
    return false;
#endif
}

BigInt::BigInt() {
#if defined(FORMULAIC_HAS_GMP)
    mpz_init(val_);
#else
    fallback_val_ = 0;
#endif
}

BigInt::BigInt(int64_t val) {
#if defined(FORMULAIC_HAS_GMP)
    mpz_init(val_);
    const bool neg = val < 0;
    const uint64_t uval = neg ? static_cast<uint64_t>(-val) : static_cast<uint64_t>(val);
    const uint32_t high = static_cast<uint32_t>(uval >> 32);
    const uint32_t low = static_cast<uint32_t>(uval & 0xFFFFFFFFULL);
    mpz_set_ui(val_, high);
    mpz_mul_2exp(val_, val_, 32);
    mpz_add_ui(val_, val_, low);
    if (neg) {
        mpz_neg(val_, val_);
    }
#else
    fallback_val_ = val;
#endif
}

BigInt::BigInt(uint64_t val) {
#if defined(FORMULAIC_HAS_GMP)
    mpz_init(val_);
    const uint32_t high = static_cast<uint32_t>(val >> 32);
    const uint32_t low = static_cast<uint32_t>(val & 0xFFFFFFFFULL);
    mpz_set_ui(val_, high);
    mpz_mul_2exp(val_, val_, 32);
    mpz_add_ui(val_, val_, low);
#else
    fallback_val_ = static_cast<int64_t>(val);
#endif
}

BigInt::BigInt(int val) : BigInt(static_cast<int64_t>(val)) {}
BigInt::BigInt(unsigned int val) : BigInt(static_cast<uint64_t>(val)) {}

BigInt::BigInt(std::string_view str, int base) {
#if defined(FORMULAIC_HAS_GMP)
    mpz_init(val_);
    std::string s(str);
    if (mpz_set_str(val_, s.c_str(), base) != 0) {
        mpz_set_ui(val_, 0);
    }
#else
    try {
        fallback_val_ = std::stoll(std::string(str), nullptr, base);
    } catch (...) {
        fallback_val_ = 0;
    }
#endif
}

BigInt::BigInt(const char* str, int base) : BigInt(std::string_view(str ? str : "0"), base) {}
BigInt::BigInt(const std::string& str, int base) : BigInt(std::string_view(str), base) {}

BigInt::BigInt(const BigInt& other) {
#if defined(FORMULAIC_HAS_GMP)
    mpz_init_set(val_, other.val_);
#else
    fallback_val_ = other.fallback_val_;
#endif
}

BigInt::BigInt(BigInt&& other) noexcept {
#if defined(FORMULAIC_HAS_GMP)
    mpz_init(val_);
    mpz_swap(val_, other.val_);
#else
    fallback_val_ = other.fallback_val_;
    other.fallback_val_ = 0;
#endif
}

BigInt& BigInt::operator=(const BigInt& other) {
    if (this != &other) {
#if defined(FORMULAIC_HAS_GMP)
        mpz_set(val_, other.val_);
#else
        fallback_val_ = other.fallback_val_;
#endif
    }
    return *this;
}

BigInt& BigInt::operator=(BigInt&& other) noexcept {
    if (this != &other) {
#if defined(FORMULAIC_HAS_GMP)
        mpz_swap(val_, other.val_);
#else
        fallback_val_ = other.fallback_val_;
        other.fallback_val_ = 0;
#endif
    }
    return *this;
}

BigInt::~BigInt() {
#if defined(FORMULAIC_HAS_GMP)
    mpz_clear(val_);
#endif
}

BigInt BigInt::operator+(const BigInt& other) const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_add(res.val_, val_, other.val_);
#else
    res.fallback_val_ = fallback_val_ + other.fallback_val_;
#endif
    return res;
}

BigInt BigInt::operator-(const BigInt& other) const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_sub(res.val_, val_, other.val_);
#else
    res.fallback_val_ = fallback_val_ - other.fallback_val_;
#endif
    return res;
}

BigInt BigInt::operator*(const BigInt& other) const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_mul(res.val_, val_, other.val_);
#else
    res.fallback_val_ = fallback_val_ * other.fallback_val_;
#endif
    return res;
}

BigInt BigInt::operator/(const BigInt& other) const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    if (mpz_sgn(other.val_) == 0) {
        throw std::domain_error("Division by zero in BigInt::operator/");
    }
    mpz_tdiv_q(res.val_, val_, other.val_);
#else
    if (other.fallback_val_ == 0) throw std::domain_error("Division by zero");
    res.fallback_val_ = fallback_val_ / other.fallback_val_;
#endif
    return res;
}

BigInt BigInt::operator%(const BigInt& other) const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    if (mpz_sgn(other.val_) == 0) {
        throw std::domain_error("Modulo by zero in BigInt::operator%");
    }
    mpz_tdiv_r(res.val_, val_, other.val_);
#else
    if (other.fallback_val_ == 0) throw std::domain_error("Modulo by zero");
    res.fallback_val_ = fallback_val_ % other.fallback_val_;
#endif
    return res;
}

BigInt& BigInt::operator+=(const BigInt& other) {
#if defined(FORMULAIC_HAS_GMP)
    mpz_add(val_, val_, other.val_);
#else
    fallback_val_ += other.fallback_val_;
#endif
    return *this;
}

BigInt& BigInt::operator-=(const BigInt& other) {
#if defined(FORMULAIC_HAS_GMP)
    mpz_sub(val_, val_, other.val_);
#else
    fallback_val_ -= other.fallback_val_;
#endif
    return *this;
}

BigInt& BigInt::operator*=(const BigInt& other) {
#if defined(FORMULAIC_HAS_GMP)
    mpz_mul(val_, val_, other.val_);
#else
    fallback_val_ *= other.fallback_val_;
#endif
    return *this;
}

BigInt& BigInt::operator/=(const BigInt& other) {
#if defined(FORMULAIC_HAS_GMP)
    if (mpz_sgn(other.val_) == 0) {
        throw std::domain_error("Division by zero in BigInt::operator/=");
    }
    mpz_tdiv_q(val_, val_, other.val_);
#else
    if (other.fallback_val_ == 0) throw std::domain_error("Division by zero");
    fallback_val_ /= other.fallback_val_;
#endif
    return *this;
}

BigInt& BigInt::operator%=(const BigInt& other) {
#if defined(FORMULAIC_HAS_GMP)
    if (mpz_sgn(other.val_) == 0) {
        throw std::domain_error("Modulo by zero in BigInt::operator%=");
    }
    mpz_tdiv_r(val_, val_, other.val_);
#else
    if (other.fallback_val_ == 0) throw std::domain_error("Modulo by zero");
    fallback_val_ %= other.fallback_val_;
#endif
    return *this;
}

BigInt BigInt::operator-() const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_neg(res.val_, val_);
#else
    res.fallback_val_ = -fallback_val_;
#endif
    return res;
}

BigInt BigInt::operator+() const {
    return *this;
}

BigInt& BigInt::operator++() {
    *this += 1;
    return *this;
}

BigInt BigInt::operator++(int) {
    BigInt tmp(*this);
    ++(*this);
    return tmp;
}

BigInt& BigInt::operator--() {
    *this -= 1;
    return *this;
}

BigInt BigInt::operator--(int) {
    BigInt tmp(*this);
    --(*this);
    return tmp;
}

BigInt BigInt::operator&(const BigInt& other) const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_and(res.val_, val_, other.val_);
#else
    res.fallback_val_ = fallback_val_ & other.fallback_val_;
#endif
    return res;
}

BigInt BigInt::operator|(const BigInt& other) const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_ior(res.val_, val_, other.val_);
#else
    res.fallback_val_ = fallback_val_ | other.fallback_val_;
#endif
    return res;
}

BigInt BigInt::operator^(const BigInt& other) const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_xor(res.val_, val_, other.val_);
#else
    res.fallback_val_ = fallback_val_ ^ other.fallback_val_;
#endif
    return res;
}

BigInt BigInt::operator~() const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_com(res.val_, val_);
#else
    res.fallback_val_ = ~fallback_val_;
#endif
    return res;
}

BigInt BigInt::operator<<(unsigned long shift) const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_mul_2exp(res.val_, val_, shift);
#else
    res.fallback_val_ = fallback_val_ << shift;
#endif
    return res;
}

BigInt BigInt::operator>>(unsigned long shift) const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_fdiv_q_2exp(res.val_, val_, shift);
#else
    res.fallback_val_ = fallback_val_ >> shift;
#endif
    return res;
}

BigInt& BigInt::operator&=(const BigInt& other) {
    *this = *this & other;
    return *this;
}

BigInt& BigInt::operator|=(const BigInt& other) {
    *this = *this | other;
    return *this;
}

BigInt& BigInt::operator^=(const BigInt& other) {
    *this = *this ^ other;
    return *this;
}

BigInt& BigInt::operator<<=(unsigned long shift) {
#if defined(FORMULAIC_HAS_GMP)
    mpz_mul_2exp(val_, val_, shift);
#else
    fallback_val_ <<= shift;
#endif
    return *this;
}

BigInt& BigInt::operator>>=(unsigned long shift) {
#if defined(FORMULAIC_HAS_GMP)
    mpz_fdiv_q_2exp(val_, val_, shift);
#else
    fallback_val_ >>= shift;
#endif
    return *this;
}

bool BigInt::operator==(const BigInt& other) const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_cmp(val_, other.val_) == 0;
#else
    return fallback_val_ == other.fallback_val_;
#endif
}

bool BigInt::operator!=(const BigInt& other) const noexcept {
    return !(*this == other);
}

bool BigInt::operator<(const BigInt& other) const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_cmp(val_, other.val_) < 0;
#else
    return fallback_val_ < other.fallback_val_;
#endif
}

bool BigInt::operator<=(const BigInt& other) const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_cmp(val_, other.val_) <= 0;
#else
    return fallback_val_ <= other.fallback_val_;
#endif
}

bool BigInt::operator>(const BigInt& other) const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_cmp(val_, other.val_) > 0;
#else
    return fallback_val_ > other.fallback_val_;
#endif
}

bool BigInt::operator>=(const BigInt& other) const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_cmp(val_, other.val_) >= 0;
#else
    return fallback_val_ >= other.fallback_val_;
#endif
}

std::strong_ordering BigInt::operator<=>(const BigInt& other) const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    const int c = mpz_cmp(val_, other.val_);
    if (c < 0) return std::strong_ordering::less;
    if (c > 0) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
#else
    if (fallback_val_ < other.fallback_val_) return std::strong_ordering::less;
    if (fallback_val_ > other.fallback_val_) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
#endif
}

std::string BigInt::to_string(int base) const {
#if defined(FORMULAIC_HAS_GMP)
    char* str = mpz_get_str(nullptr, base, val_);
    if (!str) return "0";
    std::string s(str);
    free(str);
    return s;
#else
    return std::to_string(fallback_val_);
#endif
}

int64_t BigInt::to_int64() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    char* str = mpz_get_str(nullptr, 10, val_);
    if (!str) return 0;
    int64_t res = std::strtoll(str, nullptr, 10);
    free(str);
    return res;
#else
    return fallback_val_;
#endif
}

double BigInt::to_double() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_get_d(val_);
#else
    return static_cast<double>(fallback_val_);
#endif
}

bool BigInt::is_zero() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_sgn(val_) == 0;
#else
    return fallback_val_ == 0;
#endif
}

bool BigInt::is_negative() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_sgn(val_) < 0;
#else
    return fallback_val_ < 0;
#endif
}

bool BigInt::is_positive() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_sgn(val_) > 0;
#else
    return fallback_val_ > 0;
#endif
}

bool BigInt::is_even() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_tstbit(val_, 0) == 0;
#else
    return (fallback_val_ % 2) == 0;
#endif
}

bool BigInt::is_odd() const noexcept {
    return !is_even();
}

int BigInt::sign() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_sgn(val_);
#else
    if (fallback_val_ > 0) return 1;
    if (fallback_val_ < 0) return -1;
    return 0;
#endif
}

size_t BigInt::bit_length() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_sizeinbase(val_, 2);
#else
    return 64;
#endif
}

BigInt BigInt::abs() const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_abs(res.val_, val_);
#else
    res.fallback_val_ = std::abs(fallback_val_);
#endif
    return res;
}

BigInt BigInt::gcd(const BigInt& a, const BigInt& b) {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_gcd(res.val_, a.val_, b.val_);
#else
    res.fallback_val_ = 1;
#endif
    return res;
}

BigInt BigInt::lcm(const BigInt& a, const BigInt& b) {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_lcm(res.val_, a.val_, b.val_);
#else
    res.fallback_val_ = 1;
#endif
    return res;
}

BigInt BigInt::factorial(unsigned long n) {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_fac_ui(res.val_, n);
#else
    res.fallback_val_ = 1;
#endif
    return res;
}

BigInt BigInt::binomial(unsigned long n, unsigned long k) {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_bin_uiui(res.val_, n, k);
#else
    res.fallback_val_ = 1;
#endif
    return res;
}

// Fast doubling Fibonacci: F(2k) = F(k)[2F(k+1) - F(k)], F(2k+1) = F(k+1)^2 + F(k)^2
static std::pair<BigInt, BigInt> fib_doubling(unsigned long n) {
    if (n == 0) return {BigInt(0), BigInt(1)};
    auto [a, b] = fib_doubling(n >> 1);
    BigInt c = a * (b * 2 - a);
    BigInt d = a * a + b * b;
    if ((n & 1) == 0) {
        return {c, d};
    } else {
        return {d, c + d};
    }
}

BigInt BigInt::fibonacci(unsigned long n) {
    return fib_doubling(n).first;
}

BigInt BigInt::pow(const BigInt& base, unsigned long exp) {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_pow_ui(res.val_, base.val_, exp);
#else
    res.fallback_val_ = 1;
#endif
    return res;
}

BigInt BigInt::pow_mod(const BigInt& base, const BigInt& exp, const BigInt& mod) {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_powm(res.val_, base.val_, exp.val_, mod.val_);
#else
    res.fallback_val_ = 1;
#endif
    return res;
}

BigInt BigInt::sqrt(const BigInt& n) {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    if (mpz_sgn(n.val_) < 0) {
        throw std::domain_error("Square root of negative number in BigInt::sqrt");
    }
    mpz_sqrt(res.val_, n.val_);
#else
    res.fallback_val_ = 0;
#endif
    return res;
}

bool BigInt::is_probab_prime(int reps) const {
#if defined(FORMULAIC_HAS_GMP)
    return mpz_probab_prime_p(val_, reps) > 0;
#else
    return false;
#endif
}

BigInt BigInt::next_prime() const {
    BigInt p = *this;
    if (p <= 1) return BigInt(2);
    if (p.is_even()) p += 1;
    else p += 2;
    while (!p.is_probab_prime(25)) {
        p += 2;
    }
    return p;
}

std::ostream& operator<<(std::ostream& os, const BigInt& bi) {
    os << bi.to_string();
    return os;
}

} // namespace formulaic::math
