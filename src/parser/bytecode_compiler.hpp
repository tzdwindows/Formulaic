#pragma once

#include <Formulaic/core/error.hpp>
#include <Formulaic/parser/ast.hpp>
#include <Formulaic/parser/bytecode.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace formulaic {

class BytecodeCompiler {
public:
    explicit BytecodeCompiler(std::vector<std::string> variable_names);

    Result<BytecodeProgram> compile(const ASTNode& root);

private:
    Result<void> compile_node(const ASTNode& node);
    uint16_t add_constant(double value);
    Result<uint16_t> get_variable_index(std::string_view name, const SourceLocation& loc);

    void emit(Opcode op, uint16_t operand = 0);
    void adjust_stack(int delta);

    BytecodeProgram program_;
    std::unordered_map<std::string, uint16_t> var_to_index_;
    int current_stack_depth_{0};
    size_t max_stack_depth_{0};
};

} // namespace formulaic
