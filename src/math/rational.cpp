#include <Formulaic/math/rational.hpp>
#include <Formulaic/math/gmp_types.hpp>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

namespace formulaic::math {

Rational::Rational() {
#if defined(FORMULAIC_HAS_GMP)
    mpq_init(val_);
    mpq_set_ui(val_, 0, 1);
#else
    fallback_num_ = 0;
    fallback_den_ = 1;
#endif
}

Rational::Rational(int64_t num, int64_t den) {
#if defined(FORMULAIC_HAS_GMP)
    mpq_init(val_);
    if (den == 0) {
        throw std::domain_error("Denominator cannot be zero in Rational");
    }
    BigInt b_num(num);
    BigInt b_den(den);
    mpq_set_num(val_, b_num.raw());
    mpq_set_den(val_, b_den.raw());
    canonicalize();
#else
    if (den == 0) throw std::domain_error("Denominator cannot be zero");
    fallback_num_ = num;
    fallback_den_ = den;
#endif
}

Rational::Rational(const BigInt& num, const BigInt& den) {
#if defined(FORMULAIC_HAS_GMP)
    mpq_init(val_);
    if (den.is_zero()) {
        throw std::domain_error("Denominator cannot be zero in Rational");
    }
    mpq_set_num(val_, num.raw());
    mpq_set_den(val_, den.raw());
    canonicalize();
#else
    if (den.to_int64() == 0) throw std::domain_error("Denominator cannot be zero");
    fallback_num_ = num.to_int64();
    fallback_den_ = den.to_int64();
#endif
}

Rational::Rational(std::string_view str, int base) {
#if defined(FORMULAIC_HAS_GMP)
    mpq_init(val_);
    std::string s(str);
    // If no '/', append "/1" so mpq_set_str parses pure integers as rationals
    if (s.find('/') == std::string::npos) {
        s += "/1";
    }
    if (mpq_set_str(val_, s.c_str(), base) != 0) {
        mpq_set_ui(val_, 0, 1);
    } else {
        canonicalize();
    }
#else
    fallback_num_ = 0;
    fallback_den_ = 1;
#endif
}

Rational::Rational(const char* str, int base) : Rational(std::string_view(str ? str : "0"), base) {}
Rational::Rational(const std::string& str, int base) : Rational(std::string_view(str), base) {}

Rational::Rational(const Rational& other) {
#if defined(FORMULAIC_HAS_GMP)
    mpq_init(val_);
    mpq_set(val_, other.val_);
#else
    fallback_num_ = other.fallback_num_;
    fallback_den_ = other.fallback_den_;
#endif
}

Rational::Rational(Rational&& other) noexcept {
#if defined(FORMULAIC_HAS_GMP)
    mpq_init(val_);
    mpq_swap(val_, other.val_);
#else
    fallback_num_ = other.fallback_num_;
    fallback_den_ = other.fallback_den_;
    other.fallback_num_ = 0;
    other.fallback_den_ = 1;
#endif
}

Rational& Rational::operator=(const Rational& other) {
    if (this != &other) {
#if defined(FORMULAIC_HAS_GMP)
        mpq_set(val_, other.val_);
#else
        fallback_num_ = other.fallback_num_;
        fallback_den_ = other.fallback_den_;
#endif
    }
    return *this;
}

Rational& Rational::operator=(Rational&& other) noexcept {
    if (this != &other) {
#if defined(FORMULAIC_HAS_GMP)
        mpq_swap(val_, other.val_);
#else
        fallback_num_ = other.fallback_num_;
        fallback_den_ = other.fallback_den_;
        other.fallback_num_ = 0;
        other.fallback_den_ = 1;
#endif
    }
    return *this;
}

Rational::~Rational() {
#if defined(FORMULAIC_HAS_GMP)
    mpq_clear(val_);
#endif
}

void Rational::canonicalize() {
#if defined(FORMULAIC_HAS_GMP)
    mpq_canonicalize(val_);
#endif
}

Rational Rational::operator+(const Rational& other) const {
    Rational res;
#if defined(FORMULAIC_HAS_GMP)
    mpq_add(res.val_, val_, other.val_);
#else
    res.fallback_num_ = fallback_num_ * other.fallback_den_ + other.fallback_num_ * fallback_den_;
    res.fallback_den_ = fallback_den_ * other.fallback_den_;
#endif
    return res;
}

Rational Rational::operator-(const Rational& other) const {
    Rational res;
#if defined(FORMULAIC_HAS_GMP)
    mpq_sub(res.val_, val_, other.val_);
#else
    res.fallback_num_ = fallback_num_ * other.fallback_den_ - other.fallback_num_ * fallback_den_;
    res.fallback_den_ = fallback_den_ * other.fallback_den_;
#endif
    return res;
}

Rational Rational::operator*(const Rational& other) const {
    Rational res;
#if defined(FORMULAIC_HAS_GMP)
    mpq_mul(res.val_, val_, other.val_);
#else
    res.fallback_num_ = fallback_num_ * other.fallback_num_;
    res.fallback_den_ = fallback_den_ * other.fallback_den_;
#endif
    return res;
}

Rational Rational::operator/(const Rational& other) const {
    Rational res;
#if defined(FORMULAIC_HAS_GMP)
    if (mpq_sgn(other.val_) == 0) {
        throw std::domain_error("Division by zero in Rational::operator/");
    }
    mpq_div(res.val_, val_, other.val_);
#else
    if (other.fallback_num_ == 0) throw std::domain_error("Division by zero");
    res.fallback_num_ = fallback_num_ * other.fallback_den_;
    res.fallback_den_ = fallback_den_ * other.fallback_num_;
#endif
    return res;
}

Rational& Rational::operator+=(const Rational& other) {
#if defined(FORMULAIC_HAS_GMP)
    mpq_add(val_, val_, other.val_);
#else
    *this = *this + other;
#endif
    return *this;
}

Rational& Rational::operator-=(const Rational& other) {
#if defined(FORMULAIC_HAS_GMP)
    mpq_sub(val_, val_, other.val_);
#else
    *this = *this - other;
#endif
    return *this;
}

Rational& Rational::operator*=(const Rational& other) {
#if defined(FORMULAIC_HAS_GMP)
    mpq_mul(val_, val_, other.val_);
#else
    *this = *this * other;
#endif
    return *this;
}

Rational& Rational::operator/=(const Rational& other) {
#if defined(FORMULAIC_HAS_GMP)
    if (mpq_sgn(other.val_) == 0) {
        throw std::domain_error("Division by zero in Rational::operator/=");
    }
    mpq_div(val_, val_, other.val_);
#else
    *this = *this / other;
#endif
    return *this;
}

Rational Rational::operator-() const {
    Rational res;
#if defined(FORMULAIC_HAS_GMP)
    mpq_neg(res.val_, val_);
#else
    res.fallback_num_ = -fallback_num_;
    res.fallback_den_ = fallback_den_;
#endif
    return res;
}

Rational Rational::operator+() const {
    return *this;
}

bool Rational::operator==(const Rational& other) const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpq_cmp(val_, other.val_) == 0;
#else
    return fallback_num_ * other.fallback_den_ == other.fallback_num_ * fallback_den_;
#endif
}

bool Rational::operator!=(const Rational& other) const noexcept {
    return !(*this == other);
}

bool Rational::operator<(const Rational& other) const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpq_cmp(val_, other.val_) < 0;
#else
    return fallback_num_ * other.fallback_den_ < other.fallback_num_ * fallback_den_;
#endif
}

bool Rational::operator<=(const Rational& other) const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpq_cmp(val_, other.val_) <= 0;
#else
    return fallback_num_ * other.fallback_den_ <= other.fallback_num_ * fallback_den_;
#endif
}

bool Rational::operator>(const Rational& other) const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpq_cmp(val_, other.val_) > 0;
#else
    return fallback_num_ * other.fallback_den_ > other.fallback_num_ * fallback_den_;
#endif
}

bool Rational::operator>=(const Rational& other) const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpq_cmp(val_, other.val_) >= 0;
#else
    return fallback_num_ * other.fallback_den_ >= other.fallback_num_ * fallback_den_;
#endif
}

std::strong_ordering Rational::operator<=>(const Rational& other) const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    const int c = mpq_cmp(val_, other.val_);
    if (c < 0) return std::strong_ordering::less;
    if (c > 0) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
#else
    int64_t diff = fallback_num_ * other.fallback_den_ - other.fallback_num_ * fallback_den_;
    if (diff < 0) return std::strong_ordering::less;
    if (diff > 0) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
#endif
}

BigInt Rational::num() const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_set(res.raw(), mpq_numref(val_));
#else
    res = BigInt(fallback_num_);
#endif
    return res;
}

BigInt Rational::den() const {
    BigInt res;
#if defined(FORMULAIC_HAS_GMP)
    mpz_set(res.raw(), mpq_denref(val_));
#else
    res = BigInt(fallback_den_);
#endif
    return res;
}

std::string Rational::to_string(int base) const {
#if defined(FORMULAIC_HAS_GMP)
    char* str = mpq_get_str(nullptr, base, val_);
    if (!str) return "0";
    std::string s(str);
    void (*free_func)(void*, size_t) = nullptr;
    mp_get_memory_functions(nullptr, nullptr, &free_func);
    if (free_func) {
        free_func(str, s.size() + 1);
    } else {
        free(str);
    }
    return s;
#else
    return std::to_string(fallback_num_) + "/" + std::to_string(fallback_den_);
#endif
}

double Rational::to_double() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpq_get_d(val_);
#else
    return static_cast<double>(fallback_num_) / fallback_den_;
#endif
}

bool Rational::is_zero() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpq_sgn(val_) == 0;
#else
    return fallback_num_ == 0;
#endif
}

bool Rational::is_negative() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpq_sgn(val_) < 0;
#else
    return (fallback_num_ < 0) ^ (fallback_den_ < 0);
#endif
}

bool Rational::is_positive() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpq_sgn(val_) > 0;
#else
    return (fallback_num_ > 0 && fallback_den_ > 0) || (fallback_num_ < 0 && fallback_den_ < 0);
#endif
}

int Rational::sign() const noexcept {
#if defined(FORMULAIC_HAS_GMP)
    return mpq_sgn(val_);
#else
    if (is_positive()) return 1;
    if (is_negative()) return -1;
    return 0;
#endif
}

Rational Rational::abs() const {
    Rational res;
#if defined(FORMULAIC_HAS_GMP)
    mpq_abs(res.val_, val_);
#else
    res.fallback_num_ = std::abs(fallback_num_);
    res.fallback_den_ = std::abs(fallback_den_);
#endif
    return res;
}

Rational Rational::inv() const {
    Rational res;
#if defined(FORMULAIC_HAS_GMP)
    if (mpq_sgn(val_) == 0) {
        throw std::domain_error("Cannot invert zero in Rational::inv");
    }
    mpq_inv(res.val_, val_);
#else
    if (fallback_num_ == 0) throw std::domain_error("Cannot invert zero");
    res.fallback_num_ = fallback_den_;
    res.fallback_den_ = fallback_num_;
#endif
    return res;
}

std::ostream& operator<<(std::ostream& os, const Rational& rat) {
    os << rat.to_string();
    return os;
}

} // namespace formulaic::math
