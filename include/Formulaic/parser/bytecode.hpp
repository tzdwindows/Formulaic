#pragma once

#include <Formulaic/core/export.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace formulaic {

enum class Opcode : uint8_t {
    NOP = 0,
    CONSTANT,       // operand = constant index
    LOAD_VAR,       // operand = variable index
    STORE_VAR,      // operand = variable index
    POP,            // pop top of stack

    // Arithmetic
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    POW,
    NEG,

    // Comparison & Logic
    EQ,
    NE,
    LT,
    LE,
    GT,
    GE,
    AND,
    OR,
    NOT,

    // Standard Math Functions (Single Argument)
    SIN,
    COS,
    TAN,
    ASIN,
    ACOS,
    ATAN,
    SINH,
    COSH,
    TANH,
    ASINH,
    ACOSH,
    ATANH,

    // Reciprocal Trigonometric & Hyperbolic
    SEC,
    CSC,
    COT,
    ASEC,
    ACSC,
    ACOT,
    SECH,
    CSCH,
    COTH,

    // Special & Statistical Functions
    SINC,
    ERF,
    ERFC,
    TGAMMA,
    LGAMMA,

    // Exponential & Logarithmic Variants
    EXP,
    EXP2,
    EXPM1,
    LN,
    LOG10,
    LOG2,
    LOG1P,

    // Roots & Rounding
    SQRT,
    CBRT,
    ABS,
    FLOOR,
    CEIL,
    ROUND,
    TRUNC,
    FRACT,
    SIGN,

    // Signal & Angle Transforms
    HEAVISIDE,
    RECT,
    TRI,
    DEG2RAD,
    RAD2DEG,

    // Multi-Argument Functions
    ATAN2,          // 2 args: y, x
    MIN,            // 2 args
    MAX,            // 2 args
    HYPOT,          // 2 args
    COPYSIGN,       // 2 args
    REMAINDER,      // 2 args
    BETA,           // 2 args
    STEP,           // 2 args: edge, x
    CLAMP,          // 3 args: x, min, max
    SMOOTHSTEP,     // 3 args: edge0, edge1, x
    LERP,           // 3 args: a, b, t

    // Calculus Functions
    DIFF_STEP,      // 3 args: f_plus, f_minus, h
    GRADIENT2D,     // 2 args: dx, dy
    CURVATURE,      // 2 args: yp, ypp
    TRAPZ,          // 3 args: y0, y1, dx
    SIMPSON,        // 4 args: y0, y1, y2, h
    EULER,          // 3 args: y, dydt, dt
    RK4,            // 6 args: y, k1, k2, k3, k4, dt
    LAPLACIAN2D,    // 2 args: f_xx, f_yy

    // Spectral & Windowing (FFT) Functions
    HANN,           // 1 arg
    HAMMING,        // 1 arg
    BLACKMAN,       // 1 arg
    BARTLETT,       // 1 arg
    FLATTOP,        // 1 arg
    SQUARE_WAVE,    // 1 arg
    SAWTOOTH_WAVE,  // 1 arg
    TRIANGLE_WAVE,  // 1 arg
    DIRICHLET,      // 2 args: n, x
    GAUSSIAN,       // 3 args: x, mu, sigma
    CHIRP,          // 4 args: t, f0, t1, f1

    RET
};

#pragma pack(push, 2)
struct Instruction {
    Opcode op{Opcode::NOP};
    uint16_t operand{0};
};
#pragma pack(pop)

struct FORMULAIC_API BytecodeProgram {
    std::vector<Instruction> instructions;
    std::vector<double> constants;
    std::vector<std::string> variable_names;
    size_t max_stack_depth{16};

    [[nodiscard]] std::string disassemble() const;
};

[[nodiscard]] constexpr std::string_view opcode_name(Opcode op) noexcept {
    switch (op) {
        case Opcode::NOP: return "NOP";
        case Opcode::CONSTANT: return "CONSTANT";
        case Opcode::LOAD_VAR: return "LOAD_VAR";
        case Opcode::STORE_VAR: return "STORE_VAR";
        case Opcode::POP: return "POP";
        case Opcode::ADD: return "ADD";
        case Opcode::SUB: return "SUB";
        case Opcode::MUL: return "MUL";
        case Opcode::DIV: return "DIV";
        case Opcode::MOD: return "MOD";
        case Opcode::POW: return "POW";
        case Opcode::NEG: return "NEG";
        case Opcode::EQ: return "EQ";
        case Opcode::NE: return "NE";
        case Opcode::LT: return "LT";
        case Opcode::LE: return "LE";
        case Opcode::GT: return "GT";
        case Opcode::GE: return "GE";
        case Opcode::AND: return "AND";
        case Opcode::OR: return "OR";
        case Opcode::NOT: return "NOT";
        case Opcode::SIN: return "SIN";
        case Opcode::COS: return "COS";
        case Opcode::TAN: return "TAN";
        case Opcode::ASIN: return "ASIN";
        case Opcode::ACOS: return "ACOS";
        case Opcode::ATAN: return "ATAN";
        case Opcode::SINH: return "SINH";
        case Opcode::COSH: return "COSH";
        case Opcode::TANH: return "TANH";
        case Opcode::ASINH: return "ASINH";
        case Opcode::ACOSH: return "ACOSH";
        case Opcode::ATANH: return "ATANH";
        case Opcode::SEC: return "SEC";
        case Opcode::CSC: return "CSC";
        case Opcode::COT: return "COT";
        case Opcode::ASEC: return "ASEC";
        case Opcode::ACSC: return "ACSC";
        case Opcode::ACOT: return "ACOT";
        case Opcode::SECH: return "SECH";
        case Opcode::CSCH: return "CSCH";
        case Opcode::COTH: return "COTH";
        case Opcode::SINC: return "SINC";
        case Opcode::ERF: return "ERF";
        case Opcode::ERFC: return "ERFC";
        case Opcode::TGAMMA: return "TGAMMA";
        case Opcode::LGAMMA: return "LGAMMA";
        case Opcode::EXP: return "EXP";
        case Opcode::EXP2: return "EXP2";
        case Opcode::EXPM1: return "EXPM1";
        case Opcode::LN: return "LN";
        case Opcode::LOG10: return "LOG10";
        case Opcode::LOG2: return "LOG2";
        case Opcode::LOG1P: return "LOG1P";
        case Opcode::SQRT: return "SQRT";
        case Opcode::CBRT: return "CBRT";
        case Opcode::ABS: return "ABS";
        case Opcode::FLOOR: return "FLOOR";
        case Opcode::CEIL: return "CEIL";
        case Opcode::ROUND: return "ROUND";
        case Opcode::TRUNC: return "TRUNC";
        case Opcode::FRACT: return "FRACT";
        case Opcode::SIGN: return "SIGN";
        case Opcode::HEAVISIDE: return "HEAVISIDE";
        case Opcode::RECT: return "RECT";
        case Opcode::TRI: return "TRI";
        case Opcode::DEG2RAD: return "DEG2RAD";
        case Opcode::RAD2DEG: return "RAD2DEG";
        case Opcode::ATAN2: return "ATAN2";
        case Opcode::MIN: return "MIN";
        case Opcode::MAX: return "MAX";
        case Opcode::HYPOT: return "HYPOT";
        case Opcode::COPYSIGN: return "COPYSIGN";
        case Opcode::REMAINDER: return "REMAINDER";
        case Opcode::BETA: return "BETA";
        case Opcode::STEP: return "STEP";
        case Opcode::CLAMP: return "CLAMP";
        case Opcode::SMOOTHSTEP: return "SMOOTHSTEP";
        case Opcode::LERP: return "LERP";
        case Opcode::DIFF_STEP: return "DIFF_STEP";
        case Opcode::GRADIENT2D: return "GRADIENT2D";
        case Opcode::CURVATURE: return "CURVATURE";
        case Opcode::TRAPZ: return "TRAPZ";
        case Opcode::SIMPSON: return "SIMPSON";
        case Opcode::EULER: return "EULER";
        case Opcode::RK4: return "RK4";
        case Opcode::LAPLACIAN2D: return "LAPLACIAN2D";
        case Opcode::HANN: return "HANN";
        case Opcode::HAMMING: return "HAMMING";
        case Opcode::BLACKMAN: return "BLACKMAN";
        case Opcode::BARTLETT: return "BARTLETT";
        case Opcode::FLATTOP: return "FLATTOP";
        case Opcode::SQUARE_WAVE: return "SQUARE_WAVE";
        case Opcode::SAWTOOTH_WAVE: return "SAWTOOTH_WAVE";
        case Opcode::TRIANGLE_WAVE: return "TRIANGLE_WAVE";
        case Opcode::DIRICHLET: return "DIRICHLET";
        case Opcode::GAUSSIAN: return "GAUSSIAN";
        case Opcode::CHIRP: return "CHIRP";
        case Opcode::RET: return "RET";
        default: return "UNKNOWN";
    }
}

} // namespace formulaic
