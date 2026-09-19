#include <Formulaic/math/gmp_evaluator.hpp>
#include <cctype>
#include <string>
#include <vector>

namespace formulaic::math {

namespace {

inline Diagnostic make_error(ErrorCode code, std::string msg, size_t offset = 0) {
    return Diagnostic{code, std::move(msg), SourceLocation{1, offset + 1, offset}};
}

class IntParser {
public:
    explicit IntParser(std::string_view input) : input_(input), pos_(0) {
        skip_whitespace();
    }

    Result<BigInt> parse() {
        auto res = parse_expression();
        if (!res) return res;
        if (pos_ < input_.size()) {
            return make_error(ErrorCode::UnexpectedToken, "Unexpected characters at end of integer expression", pos_);
        }
        return res;
    }

private:
    std::string_view input_;
    size_t pos_{0};

    void skip_whitespace() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
            ++pos_;
        }
    }

    char peek() const {
        return pos_ < input_.size() ? input_[pos_] : '\0';
    }

    char get() {
        char c = peek();
        if (pos_ < input_.size()) ++pos_;
        skip_whitespace();
        return c;
    }

    bool match(char c) {
        if (peek() == c) {
            get();
            return true;
        }
        return false;
    }

    // expression := term (('+' | '-') term)*
    Result<BigInt> parse_expression() {
        auto left = parse_term();
        if (!left) return left;
        BigInt res = left.value();

        while (peek() == '+' || peek() == '-') {
            char op = get();
            auto right = parse_term();
            if (!right) return right;
            if (op == '+') res += right.value();
            else res -= right.value();
        }
        return res;
    }

    // term := power (('*' | '/' | '%') power)*
    Result<BigInt> parse_term() {
        auto left = parse_power();
        if (!left) return left;
        BigInt res = left.value();

        while (peek() == '*' || peek() == '/' || peek() == '%') {
            char op = get();
            auto right = parse_power();
            if (!right) return right;
            try {
                if (op == '*') res *= right.value();
                else if (op == '/') res /= right.value();
                else res %= right.value();
            } catch (const std::exception& e) {
                return make_error(ErrorCode::DivisionByZero, e.what(), pos_);
            }
        }
        return res;
    }

    // power := factor ('^' power)?
    Result<BigInt> parse_power() {
        auto base = parse_factor();
        if (!base) return base;

        if (match('^')) {
            auto exp = parse_power();
            if (!exp) return exp;
            if (exp.value().is_negative()) {
                return make_error(ErrorCode::EvaluationError, "Negative exponent in integer power", pos_);
            }
            return BigInt::pow(base.value(), static_cast<unsigned long>(exp.value().to_int64()));
        }
        return base;
    }

    // factor := ('+' | '-')? primary ('!')*
    Result<BigInt> parse_factor() {
        if (match('+')) return parse_factor();
        if (match('-')) {
            auto res = parse_factor();
            if (!res) return res;
            return -res.value();
        }

        auto prim = parse_primary();
        if (!prim) return prim;
        BigInt res = prim.value();

        // Handle postfix '!' (factorial)
        while (match('!')) {
            if (res.is_negative()) {
                return make_error(ErrorCode::EvaluationError, "Factorial of negative integer", pos_);
            }
            res = BigInt::factorial(static_cast<unsigned long>(res.to_int64()));
        }
        return res;
    }

    // primary := number | '(' expression ')' | identifier '(' args ')'
    Result<BigInt> parse_primary() {
        if (match('(')) {
            auto res = parse_expression();
            if (!res) return res;
            if (!match(')')) {
                return make_error(ErrorCode::MissingParenthesis, "Expected closing ')'", pos_);
            }
            return res;
        }

        if (std::isdigit(static_cast<unsigned char>(peek()))) {
            size_t start = pos_;
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
            std::string_view num_str = input_.substr(start, pos_ - start);
            skip_whitespace();
            return BigInt(num_str);
        }

        if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_') {
            size_t start = pos_;
            while (pos_ < input_.size() && (std::isalnum(static_cast<unsigned char>(input_[pos_])) || input_[pos_] == '_')) {
                ++pos_;
            }
            std::string fn_name(input_.substr(start, pos_ - start));
            skip_whitespace();

            if (!match('(')) {
                return make_error(ErrorCode::UnexpectedToken, "Expected '(' after function name " + fn_name, pos_);
            }

            std::vector<BigInt> args;
            if (!match(')')) {
                while (true) {
                    auto arg = parse_expression();
                    if (!arg) return arg;
                    args.push_back(arg.value());
                    if (match(')')) break;
                    if (!match(',')) {
                        return make_error(ErrorCode::UnexpectedToken, "Expected ',' or ')' in argument list", pos_);
                    }
                }
            }

            if (fn_name == "gcd") {
                if (args.size() != 2) return make_error(ErrorCode::InvalidArity, "gcd requires 2 arguments", start);
                return BigInt::gcd(args[0], args[1]);
            } else if (fn_name == "lcm") {
                if (args.size() != 2) return make_error(ErrorCode::InvalidArity, "lcm requires 2 arguments", start);
                return BigInt::lcm(args[0], args[1]);
            } else if (fn_name == "fact" || fn_name == "factorial") {
                if (args.size() != 1) return make_error(ErrorCode::InvalidArity, "factorial requires 1 argument", start);
                if (args[0].is_negative()) return make_error(ErrorCode::EvaluationError, "Factorial of negative integer", start);
                return BigInt::factorial(static_cast<unsigned long>(args[0].to_int64()));
            } else if (fn_name == "fib" || fn_name == "fibonacci") {
                if (args.size() != 1) return make_error(ErrorCode::InvalidArity, "fibonacci requires 1 argument", start);
                if (args[0].is_negative()) return make_error(ErrorCode::EvaluationError, "Fibonacci of negative index", start);
                return BigInt::fibonacci(static_cast<unsigned long>(args[0].to_int64()));
            } else if (fn_name == "bin" || fn_name == "binomial" || fn_name == "ncr") {
                if (args.size() != 2) return make_error(ErrorCode::InvalidArity, "binomial requires 2 arguments", start);
                return BigInt::binomial(static_cast<unsigned long>(args[0].to_int64()), static_cast<unsigned long>(args[1].to_int64()));
            } else if (fn_name == "sqrt") {
                if (args.size() != 1) return make_error(ErrorCode::InvalidArity, "sqrt requires 1 argument", start);
                try {
                    return BigInt::sqrt(args[0]);
                } catch (const std::exception& e) {
                    return make_error(ErrorCode::EvaluationError, e.what(), start);
                }
            } else if (fn_name == "pow_mod") {
                if (args.size() != 3) return make_error(ErrorCode::InvalidArity, "pow_mod requires 3 arguments (base, exp, mod)", start);
                return BigInt::pow_mod(args[0], args[1], args[2]);
            } else if (fn_name == "abs") {
                if (args.size() != 1) return make_error(ErrorCode::InvalidArity, "abs requires 1 argument", start);
                return args[0].abs();
            } else {
                return make_error(ErrorCode::UnknownIdentifier, "Unknown integer function: " + fn_name, start);
            }
        }

        return make_error(ErrorCode::UnexpectedToken, "Unexpected character in integer expression", pos_);
    }
};

class RationalParser {
public:
    explicit RationalParser(std::string_view input) : input_(input), pos_(0) {
        skip_whitespace();
    }

    Result<Rational> parse() {
        auto res = parse_expression();
        if (!res) return res;
        if (pos_ < input_.size()) {
            return make_error(ErrorCode::UnexpectedToken, "Unexpected characters at end of rational expression", pos_);
        }
        return res;
    }

private:
    std::string_view input_;
    size_t pos_{0};

    void skip_whitespace() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
            ++pos_;
        }
    }

    char peek() const {
        return pos_ < input_.size() ? input_[pos_] : '\0';
    }

    char get() {
        char c = peek();
        if (pos_ < input_.size()) ++pos_;
        skip_whitespace();
        return c;
    }

    bool match(char c) {
        if (peek() == c) {
            get();
            return true;
        }
        return false;
    }

    Result<Rational> parse_expression() {
        auto left = parse_term();
        if (!left) return left;
        Rational res = left.value();

        while (peek() == '+' || peek() == '-') {
            char op = get();
            auto right = parse_term();
            if (!right) return right;
            if (op == '+') res += right.value();
            else res -= right.value();
        }
        return res;
    }

    Result<Rational> parse_term() {
        auto left = parse_factor();
        if (!left) return left;
        Rational res = left.value();

        while (peek() == '*' || peek() == '/') {
            char op = get();
            auto right = parse_factor();
            if (!right) return right;
            try {
                if (op == '*') res *= right.value();
                else res /= right.value();
            } catch (const std::exception& e) {
                return make_error(ErrorCode::DivisionByZero, e.what(), pos_);
            }
        }
        return res;
    }

    Result<Rational> parse_factor() {
        if (match('+')) return parse_factor();
        if (match('-')) {
            auto res = parse_factor();
            if (!res) return res;
            return -res.value();
        }

        if (match('(')) {
            auto res = parse_expression();
            if (!res) return res;
            if (!match(')')) {
                return make_error(ErrorCode::MissingParenthesis, "Expected ')'", pos_);
            }
            return res;
        }

        if (std::isdigit(static_cast<unsigned char>(peek()))) {
            size_t start = pos_;
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
            std::string num_str(input_.substr(start, pos_ - start));
            skip_whitespace();
            return Rational(num_str);
        }

        if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_') {
            size_t start = pos_;
            while (pos_ < input_.size() && (std::isalnum(static_cast<unsigned char>(input_[pos_])) || input_[pos_] == '_')) {
                ++pos_;
            }
            std::string fn(input_.substr(start, pos_ - start));
            skip_whitespace();
            if (!match('(')) {
                return make_error(ErrorCode::UnexpectedToken, "Expected '(' after " + fn, pos_);
            }
            auto arg = parse_expression();
            if (!arg) return arg;
            if (!match(')')) {
                return make_error(ErrorCode::MissingParenthesis, "Expected ')'", pos_);
            }
            if (fn == "abs") return arg.value().abs();
            if (fn == "inv") {
                try {
                    return arg.value().inv();
                } catch (const std::exception& e) {
                    return make_error(ErrorCode::DivisionByZero, e.what(), start);
                }
            }
            return make_error(ErrorCode::UnknownIdentifier, "Unknown rational function: " + fn, start);
        }

        return make_error(ErrorCode::UnexpectedToken, "Unexpected character in rational expression", pos_);
    }
};

} // namespace

Result<BigInt> GmpEvaluator::eval_int(std::string_view expr_str) {
    IntParser parser(expr_str);
    return parser.parse();
}

Result<Rational> GmpEvaluator::eval_rational(std::string_view expr_str) {
    RationalParser parser(expr_str);
    return parser.parse();
}

} // namespace formulaic::math
