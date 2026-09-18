#include "bytecode_vm.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace formulaic {

double BytecodeVM::evaluate(
    const BytecodeProgram& program,
    std::span<const double> variables
) noexcept {
    constexpr size_t kMaxStack = 64;
    double stack[kMaxStack];
    size_t sp = 0;

    const Instruction* const instructions = program.instructions.data();
    const size_t inst_count = program.instructions.size();
    const double* const constants = program.constants.data();
    const double* const vars = variables.data();
    const size_t var_count = variables.size();

    for (size_t ip = 0; ip < inst_count; ++ip) {
        const Instruction& inst = instructions[ip];

        switch (inst.op) {
            case Opcode::NOP:
                break;

            case Opcode::CONSTANT:
                if (sp < kMaxStack) {
                    stack[sp++] = constants[inst.operand];
                }
                break;

            case Opcode::LOAD_VAR:
                if (sp < kMaxStack) {
                    if (inst.operand < var_count) {
                        stack[sp++] = vars[inst.operand];
                    } else {
                        stack[sp++] = 0.0;
                    }
                }
                break;

            case Opcode::ADD:
                if (sp >= 2) {
                    stack[sp - 2] = stack[sp - 2] + stack[sp - 1];
                    --sp;
                }
                break;

            case Opcode::SUB:
                if (sp >= 2) {
                    stack[sp - 2] = stack[sp - 2] - stack[sp - 1];
                    --sp;
                }
                break;

            case Opcode::MUL:
                if (sp >= 2) {
                    stack[sp - 2] = stack[sp - 2] * stack[sp - 1];
                    --sp;
                }
                break;

            case Opcode::DIV:
                if (sp >= 2) {
                    const double denom = stack[sp - 1];
                    if (denom == 0.0) {
                        stack[sp - 2] = std::copysign(std::numeric_limits<double>::infinity(), stack[sp - 2]);
                    } else {
                        stack[sp - 2] = stack[sp - 2] / denom;
                    }
                    --sp;
                }
                break;

            case Opcode::MOD:
                if (sp >= 2) {
                    stack[sp - 2] = std::fmod(stack[sp - 2], stack[sp - 1]);
                    --sp;
                }
                break;

            case Opcode::POW:
                if (sp >= 2) {
                    stack[sp - 2] = std::pow(stack[sp - 2], stack[sp - 1]);
                    --sp;
                }
                break;

            case Opcode::NEG:
                if (sp >= 1) {
                    stack[sp - 1] = -stack[sp - 1];
                }
                break;

            case Opcode::EQ:
                if (sp >= 2) {
                    stack[sp - 2] = (stack[sp - 2] == stack[sp - 1]) ? 1.0 : 0.0;
                    --sp;
                }
                break;

            case Opcode::NE:
                if (sp >= 2) {
                    stack[sp - 2] = (stack[sp - 2] != stack[sp - 1]) ? 1.0 : 0.0;
                    --sp;
                }
                break;

            case Opcode::LT:
                if (sp >= 2) {
                    stack[sp - 2] = (stack[sp - 2] < stack[sp - 1]) ? 1.0 : 0.0;
                    --sp;
                }
                break;

            case Opcode::LE:
                if (sp >= 2) {
                    stack[sp - 2] = (stack[sp - 2] <= stack[sp - 1]) ? 1.0 : 0.0;
                    --sp;
                }
                break;

            case Opcode::GT:
                if (sp >= 2) {
                    stack[sp - 2] = (stack[sp - 2] > stack[sp - 1]) ? 1.0 : 0.0;
                    --sp;
                }
                break;

            case Opcode::GE:
                if (sp >= 2) {
                    stack[sp - 2] = (stack[sp - 2] >= stack[sp - 1]) ? 1.0 : 0.0;
                    --sp;
                }
                break;

            case Opcode::AND:
                if (sp >= 2) {
                    stack[sp - 2] = (stack[sp - 2] != 0.0 && stack[sp - 1] != 0.0) ? 1.0 : 0.0;
                    --sp;
                }
                break;

            case Opcode::OR:
                if (sp >= 2) {
                    stack[sp - 2] = (stack[sp - 2] != 0.0 || stack[sp - 1] != 0.0) ? 1.0 : 0.0;
                    --sp;
                }
                break;

            case Opcode::NOT:
                if (sp >= 1) {
                    stack[sp - 1] = (stack[sp - 1] == 0.0) ? 1.0 : 0.0;
                }
                break;

            case Opcode::SIN:
                if (sp >= 1) stack[sp - 1] = std::sin(stack[sp - 1]);
                break;
            case Opcode::COS:
                if (sp >= 1) stack[sp - 1] = std::cos(stack[sp - 1]);
                break;
            case Opcode::TAN:
                if (sp >= 1) stack[sp - 1] = std::tan(stack[sp - 1]);
                break;
            case Opcode::ASIN:
                if (sp >= 1) stack[sp - 1] = std::asin(stack[sp - 1]);
                break;
            case Opcode::ACOS:
                if (sp >= 1) stack[sp - 1] = std::acos(stack[sp - 1]);
                break;
            case Opcode::ATAN:
                if (sp >= 1) stack[sp - 1] = std::atan(stack[sp - 1]);
                break;
            case Opcode::ATAN2:
                if (sp >= 2) {
                    stack[sp - 2] = std::atan2(stack[sp - 2], stack[sp - 1]);
                    --sp;
                }
                break;
            case Opcode::SINH:
                if (sp >= 1) stack[sp - 1] = std::sinh(stack[sp - 1]);
                break;
            case Opcode::COSH:
                if (sp >= 1) stack[sp - 1] = std::cosh(stack[sp - 1]);
                break;
            case Opcode::TANH:
                if (sp >= 1) stack[sp - 1] = std::tanh(stack[sp - 1]);
                break;
            case Opcode::ASINH:
                if (sp >= 1) stack[sp - 1] = std::asinh(stack[sp - 1]);
                break;
            case Opcode::ACOSH:
                if (sp >= 1) stack[sp - 1] = std::acosh(stack[sp - 1]);
                break;
            case Opcode::ATANH:
                if (sp >= 1) stack[sp - 1] = std::atanh(stack[sp - 1]);
                break;
            case Opcode::SEC:
                if (sp >= 1) stack[sp - 1] = 1.0 / std::cos(stack[sp - 1]);
                break;
            case Opcode::CSC:
                if (sp >= 1) stack[sp - 1] = 1.0 / std::sin(stack[sp - 1]);
                break;
            case Opcode::COT:
                if (sp >= 1) stack[sp - 1] = 1.0 / std::tan(stack[sp - 1]);
                break;
            case Opcode::ASEC:
                if (sp >= 1) stack[sp - 1] = std::acos(1.0 / stack[sp - 1]);
                break;
            case Opcode::ACSC:
                if (sp >= 1) stack[sp - 1] = std::asin(1.0 / stack[sp - 1]);
                break;
            case Opcode::ACOT:
                if (sp >= 1) stack[sp - 1] = 1.5707963267948966 - std::atan(stack[sp - 1]);
                break;
            case Opcode::SECH:
                if (sp >= 1) stack[sp - 1] = 1.0 / std::cosh(stack[sp - 1]);
                break;
            case Opcode::CSCH:
                if (sp >= 1) stack[sp - 1] = 1.0 / std::sinh(stack[sp - 1]);
                break;
            case Opcode::COTH:
                if (sp >= 1) stack[sp - 1] = 1.0 / std::tanh(stack[sp - 1]);
                break;
            case Opcode::SINC:
                if (sp >= 1) {
                    const double v = stack[sp - 1];
                    stack[sp - 1] = (std::abs(v) < 1e-15) ? 1.0 : (std::sin(v) / v);
                }
                break;
            case Opcode::ERF:
                if (sp >= 1) stack[sp - 1] = std::erf(stack[sp - 1]);
                break;
            case Opcode::ERFC:
                if (sp >= 1) stack[sp - 1] = std::erfc(stack[sp - 1]);
                break;
            case Opcode::TGAMMA:
                if (sp >= 1) stack[sp - 1] = std::tgamma(stack[sp - 1]);
                break;
            case Opcode::LGAMMA:
                if (sp >= 1) stack[sp - 1] = std::lgamma(stack[sp - 1]);
                break;
            case Opcode::BETA:
                if (sp >= 2) {
                    const double a = stack[sp - 2];
                    const double b = stack[sp - 1];
                    stack[sp - 2] = std::exp(std::lgamma(a) + std::lgamma(b) - std::lgamma(a + b));
                    --sp;
                }
                break;
            case Opcode::EXP:
                if (sp >= 1) stack[sp - 1] = std::exp(stack[sp - 1]);
                break;
            case Opcode::EXP2:
                if (sp >= 1) stack[sp - 1] = std::exp2(stack[sp - 1]);
                break;
            case Opcode::EXPM1:
                if (sp >= 1) stack[sp - 1] = std::expm1(stack[sp - 1]);
                break;
            case Opcode::LN:
                if (sp >= 1) stack[sp - 1] = std::log(stack[sp - 1]);
                break;
            case Opcode::LOG10:
                if (sp >= 1) stack[sp - 1] = std::log10(stack[sp - 1]);
                break;
            case Opcode::LOG2:
                if (sp >= 1) stack[sp - 1] = std::log2(stack[sp - 1]);
                break;
            case Opcode::LOG1P:
                if (sp >= 1) stack[sp - 1] = std::log1p(stack[sp - 1]);
                break;
            case Opcode::SQRT:
                if (sp >= 1) stack[sp - 1] = std::sqrt(stack[sp - 1]);
                break;
            case Opcode::CBRT:
                if (sp >= 1) stack[sp - 1] = std::cbrt(stack[sp - 1]);
                break;
            case Opcode::ABS:
                if (sp >= 1) stack[sp - 1] = std::abs(stack[sp - 1]);
                break;
            case Opcode::FLOOR:
                if (sp >= 1) stack[sp - 1] = std::floor(stack[sp - 1]);
                break;
            case Opcode::CEIL:
                if (sp >= 1) stack[sp - 1] = std::ceil(stack[sp - 1]);
                break;
            case Opcode::ROUND:
                if (sp >= 1) stack[sp - 1] = std::round(stack[sp - 1]);
                break;
            case Opcode::TRUNC:
                if (sp >= 1) stack[sp - 1] = std::trunc(stack[sp - 1]);
                break;
            case Opcode::FRACT:
                if (sp >= 1) stack[sp - 1] = stack[sp - 1] - std::floor(stack[sp - 1]);
                break;
            case Opcode::SIGN:
                if (sp >= 1) {
                    const double v = stack[sp - 1];
                    stack[sp - 1] = (v > 0.0) ? 1.0 : ((v < 0.0) ? -1.0 : 0.0);
                }
                break;
            case Opcode::HEAVISIDE:
                if (sp >= 1) {
                    const double v = stack[sp - 1];
                    stack[sp - 1] = (v < 0.0) ? 0.0 : ((v > 0.0) ? 1.0 : 0.5);
                }
                break;
            case Opcode::RECT:
                if (sp >= 1) {
                    const double av = std::abs(stack[sp - 1]);
                    stack[sp - 1] = (av < 0.5) ? 1.0 : ((av == 0.5) ? 0.5 : 0.0);
                }
                break;
            case Opcode::TRI:
                if (sp >= 1) {
                    const double av = std::abs(stack[sp - 1]);
                    stack[sp - 1] = (av < 1.0) ? (1.0 - av) : 0.0;
                }
                break;
            case Opcode::DEG2RAD:
                if (sp >= 1) stack[sp - 1] = stack[sp - 1] * 0.017453292519943295;
                break;
            case Opcode::RAD2DEG:
                if (sp >= 1) stack[sp - 1] = stack[sp - 1] * 57.29577951308232;
                break;
            case Opcode::MIN:
                if (sp >= 2) {
                    stack[sp - 2] = std::min(stack[sp - 2], stack[sp - 1]);
                    --sp;
                }
                break;
            case Opcode::MAX:
                if (sp >= 2) {
                    stack[sp - 2] = std::max(stack[sp - 2], stack[sp - 1]);
                    --sp;
                }
                break;
            case Opcode::HYPOT:
                if (sp >= 2) {
                    stack[sp - 2] = std::hypot(stack[sp - 2], stack[sp - 1]);
                    --sp;
                }
                break;
            case Opcode::COPYSIGN:
                if (sp >= 2) {
                    stack[sp - 2] = std::copysign(stack[sp - 2], stack[sp - 1]);
                    --sp;
                }
                break;
            case Opcode::REMAINDER:
                if (sp >= 2) {
                    stack[sp - 2] = std::remainder(stack[sp - 2], stack[sp - 1]);
                    --sp;
                }
                break;
            case Opcode::STEP:
                if (sp >= 2) {
                    const double edge = stack[sp - 2];
                    const double x = stack[sp - 1];
                    stack[sp - 2] = (x < edge) ? 0.0 : 1.0;
                    --sp;
                }
                break;
            case Opcode::CLAMP:
                if (sp >= 3) {
                    const double val = stack[sp - 3];
                    const double lo = stack[sp - 2];
                    const double hi = stack[sp - 1];
                    stack[sp - 3] = std::clamp(val, lo, hi);
                    sp -= 2;
                }
                break;
            case Opcode::SMOOTHSTEP:
                if (sp >= 3) {
                    const double edge0 = stack[sp - 3];
                    const double edge1 = stack[sp - 2];
                    const double x = stack[sp - 1];
                    const double span = edge1 - edge0;
                    const double t = (span == 0.0) ? 0.0 : std::clamp((x - edge0) / span, 0.0, 1.0);
                    stack[sp - 3] = t * t * (3.0 - 2.0 * t);
                    sp -= 2;
                }
                break;
            case Opcode::LERP:
                if (sp >= 3) {
                    const double a = stack[sp - 3];
                    const double b = stack[sp - 2];
                    const double t = stack[sp - 1];
                    stack[sp - 3] = a + t * (b - a);
                    sp -= 2;
                }
                break;

            case Opcode::RET:
                return (sp > 0) ? stack[sp - 1] : 0.0;
        }
    }

    return (sp > 0) ? stack[sp - 1] : 0.0;
}

} // namespace formulaic
