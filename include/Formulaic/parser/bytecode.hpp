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
    EXP,
    LN,
    LOG10,
    LOG2,
    SQRT,
    CBRT,
    ABS,
    FLOOR,
    CEIL,
    ROUND,
    SIGN,

    // Multi-Argument Functions
    ATAN2,          // 2 args: y, x
    MIN,            // 2 args
    MAX,            // 2 args
    CLAMP,          // 3 args: x, min, max

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
        case Opcode::EXP: return "EXP";
        case Opcode::LN: return "LN";
        case Opcode::LOG10: return "LOG10";
        case Opcode::LOG2: return "LOG2";
        case Opcode::SQRT: return "SQRT";
        case Opcode::CBRT: return "CBRT";
        case Opcode::ABS: return "ABS";
        case Opcode::FLOOR: return "FLOOR";
        case Opcode::CEIL: return "CEIL";
        case Opcode::ROUND: return "ROUND";
        case Opcode::SIGN: return "SIGN";
        case Opcode::ATAN2: return "ATAN2";
        case Opcode::MIN: return "MIN";
        case Opcode::MAX: return "MAX";
        case Opcode::CLAMP: return "CLAMP";
        case Opcode::RET: return "RET";
        default: return "UNKNOWN";
    }
}

} // namespace formulaic
