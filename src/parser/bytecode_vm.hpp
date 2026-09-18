#pragma once

#include <Formulaic/parser/bytecode.hpp>
#include <span>

namespace formulaic {

class BytecodeVM {
public:
    // High-performance bytecode evaluator with zero heap allocations
    [[nodiscard]] static double evaluate(
        const BytecodeProgram& program,
        std::span<const double> variables
    ) noexcept;
};

} // namespace formulaic
