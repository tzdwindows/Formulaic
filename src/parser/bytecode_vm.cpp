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
            case Opcode::EXP:
                if (sp >= 1) stack[sp - 1] = std::exp(stack[sp - 1]);
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
            case Opcode::SIGN:
                if (sp >= 1) {
                    const double v = stack[sp - 1];
                    stack[sp - 1] = (v > 0.0) ? 1.0 : ((v < 0.0) ? -1.0 : 0.0);
                }
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
            case Opcode::CLAMP:
                if (sp >= 3) {
                    const double val = stack[sp - 3];
                    const double lo = stack[sp - 2];
                    const double hi = stack[sp - 1];
                    stack[sp - 3] = std::clamp(val, lo, hi);
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
