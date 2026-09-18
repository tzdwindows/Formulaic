#include "bytecode_compiler.hpp"
#include <sstream>
#include <iomanip>

namespace formulaic {

std::string BytecodeProgram::disassemble() const {
    std::ostringstream oss;
    oss << "=== Disassembly ===" << "\n";
    oss << "Variables (" << variable_names.size() << "): ";
    for (size_t i = 0; i < variable_names.size(); ++i) {
        oss << "[" << i << "]=" << variable_names[i] << " ";
    }
    oss << "\n";
    oss << "Constants (" << constants.size() << "): ";
    for (size_t i = 0; i < constants.size(); ++i) {
        oss << "[" << i << "]=" << constants[i] << " ";
    }
    oss << "\n";
    oss << "Max stack depth: " << max_stack_depth << "\n";
    oss << "Instructions (" << instructions.size() << "):\n";

    for (size_t i = 0; i < instructions.size(); ++i) {
        const auto& inst = instructions[i];
        oss << "  " << std::setw(4) << std::setfill('0') << i << ": "
            << std::setw(12) << std::setfill(' ') << std::left << opcode_name(inst.op);
        if (inst.op == Opcode::CONSTANT) {
            oss << " (idx=" << inst.operand << ", val=" << constants[inst.operand] << ")";
        } else if (inst.op == Opcode::LOAD_VAR) {
            oss << " (idx=" << inst.operand << ", name="
                << (inst.operand < variable_names.size() ? variable_names[inst.operand] : "?") << ")";
        } else if (inst.operand != 0) {
            oss << " " << inst.operand;
        }
        oss << "\n";
    }
    return oss.str();
}

BytecodeCompiler::BytecodeCompiler(std::vector<std::string> variable_names) {
    program_.variable_names = std::move(variable_names);
    for (size_t i = 0; i < program_.variable_names.size(); ++i) {
        var_to_index_[program_.variable_names[i]] = static_cast<uint16_t>(i);
    }
}

uint16_t BytecodeCompiler::add_constant(double value) {
    for (size_t i = 0; i < program_.constants.size(); ++i) {
        if (program_.constants[i] == value) {
            return static_cast<uint16_t>(i);
        }
    }
    program_.constants.push_back(value);
    return static_cast<uint16_t>(program_.constants.size() - 1);
}

Result<uint16_t> BytecodeCompiler::get_variable_index(std::string_view name, const SourceLocation& loc) {
    auto it = var_to_index_.find(std::string(name));
    if (it == var_to_index_.end()) {
        return Diagnostic{
            ErrorCode::UnknownIdentifier,
            "Variable '" + std::string(name) + "' not registered in variable list",
            loc
        };
    }
    return it->second;
}

void BytecodeCompiler::adjust_stack(int delta) {
    current_stack_depth_ += delta;
    if (current_stack_depth_ > static_cast<int>(max_stack_depth_)) {
        max_stack_depth_ = static_cast<size_t>(current_stack_depth_);
    }
}

void BytecodeCompiler::emit(Opcode op, uint16_t operand) {
    program_.instructions.push_back({op, operand});
}

Result<BytecodeProgram> BytecodeCompiler::compile(const ASTNode& root) {
    current_stack_depth_ = 0;
    max_stack_depth_ = 0;
    program_.instructions.clear();
    program_.constants.clear();

    auto res = compile_node(root);
    if (!res) return res.error();

    emit(Opcode::RET, 0);
    program_.max_stack_depth = std::max<size_t>(max_stack_depth_, 4);
    return program_;
}

Result<void> BytecodeCompiler::compile_node(const ASTNode& node) {
    if (const auto* num = dynamic_cast<const NumberNode*>(&node)) {
        uint16_t const_idx = add_constant(num->value);
        emit(Opcode::CONSTANT, const_idx);
        adjust_stack(1);
        return {};
    }

    if (const auto* var = dynamic_cast<const VariableNode*>(&node)) {
        auto idx_res = get_variable_index(var->name, var->location);
        if (!idx_res) return idx_res.error();
        emit(Opcode::LOAD_VAR, idx_res.value());
        adjust_stack(1);
        return {};
    }

    if (const auto* un = dynamic_cast<const UnaryOpNode*>(&node)) {
        auto res = compile_node(*un->operand);
        if (!res) return res;

        if (un->op == UnaryOp::Negate) {
            emit(Opcode::NEG);
        } else if (un->op == UnaryOp::Not) {
            emit(Opcode::NOT);
        }
        return {};
    }

    if (const auto* bin = dynamic_cast<const BinaryOpNode*>(&node)) {
        auto l_res = compile_node(*bin->left);
        if (!l_res) return l_res;
        auto r_res = compile_node(*bin->right);
        if (!r_res) return r_res;

        adjust_stack(-1); // Two popped, one pushed

        switch (bin->op) {
            case BinaryOp::Add: emit(Opcode::ADD); break;
            case BinaryOp::Subtract: emit(Opcode::SUB); break;
            case BinaryOp::Multiply: emit(Opcode::MUL); break;
            case BinaryOp::Divide: emit(Opcode::DIV); break;
            case BinaryOp::Modulo: emit(Opcode::MOD); break;
            case BinaryOp::Power: emit(Opcode::POW); break;
            case BinaryOp::Equal: emit(Opcode::EQ); break;
            case BinaryOp::NotEqual: emit(Opcode::NE); break;
            case BinaryOp::Less: emit(Opcode::LT); break;
            case BinaryOp::LessEqual: emit(Opcode::LE); break;
            case BinaryOp::Greater: emit(Opcode::GT); break;
            case BinaryOp::GreaterEqual: emit(Opcode::GE); break;
            case BinaryOp::LogicalAnd: emit(Opcode::AND); break;
            case BinaryOp::LogicalOr: emit(Opcode::OR); break;
        }
        return {};
    }

    if (const auto* fn = dynamic_cast<const FunctionCallNode*>(&node)) {
        for (const auto& arg : fn->args) {
            auto res = compile_node(*arg);
            if (!res) return res;
        }

        const size_t argc = fn->args.size();
        if (argc > 1) {
            adjust_stack(-static_cast<int>(argc - 1));
        }

        const auto& name = fn->name;
        if (name == "sin") emit(Opcode::SIN);
        else if (name == "cos") emit(Opcode::COS);
        else if (name == "tan") emit(Opcode::TAN);
        else if (name == "asin") emit(Opcode::ASIN);
        else if (name == "acos") emit(Opcode::ACOS);
        else if (name == "atan") emit(Opcode::ATAN);
        else if (name == "atan2") emit(Opcode::ATAN2);
        else if (name == "sinh") emit(Opcode::SINH);
        else if (name == "cosh") emit(Opcode::COSH);
        else if (name == "tanh") emit(Opcode::TANH);
        else if (name == "exp") emit(Opcode::EXP);
        else if (name == "ln" || name == "log") emit(Opcode::LN);
        else if (name == "log10") emit(Opcode::LOG10);
        else if (name == "log2") emit(Opcode::LOG2);
        else if (name == "sqrt") emit(Opcode::SQRT);
        else if (name == "cbrt") emit(Opcode::CBRT);
        else if (name == "abs") emit(Opcode::ABS);
        else if (name == "floor") emit(Opcode::FLOOR);
        else if (name == "ceil") emit(Opcode::CEIL);
        else if (name == "round") emit(Opcode::ROUND);
        else if (name == "sign") emit(Opcode::SIGN);
        else if (name == "min") emit(Opcode::MIN);
        else if (name == "max") emit(Opcode::MAX);
        else if (name == "clamp") emit(Opcode::CLAMP);
        else if (name == "pow") emit(Opcode::POW);
        else {
            return Diagnostic{
                ErrorCode::UnknownIdentifier,
                "Unknown function call during bytecode compilation: '" + name + "'",
                fn->location
            };
        }
        return {};
    }

    return Diagnostic{
        ErrorCode::InternalCompilerError,
        "Unknown AST node encountered during compilation",
        node.location
    };
}

} // namespace formulaic
