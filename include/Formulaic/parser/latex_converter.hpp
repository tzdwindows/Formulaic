#pragma once

#include <Formulaic/core/error.hpp>
#include <Formulaic/core/export.hpp>
#include <Formulaic/parser/ast.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace formulaic {

struct LatexFormatOptions {
    bool include_y_equals{false};        // If true and expression is 1D f(x), prefix with "y = "
    bool display_math_delimiters{false}; // If true, wrap in \[ ... \]
    bool greek_translation{true};        // Convert alpha -> \alpha, theta -> \theta, etc.
    bool pretty_fractions{true};         // Use \frac{a}{b} for divisions
    bool aligned_multiline{true};        // Use \begin{aligned} ... \end{aligned} for multi-line scripts
};

class FORMULAIC_API LatexConverter {
public:
    // Convert an AST node to standard LaTeX (LaTeXLive compatible)
    [[nodiscard]] static std::string to_latex(const ASTNode& node, const LatexFormatOptions& options = {});

    // Convert raw expression, script, or equation text directly to standard LaTeXLive
    [[nodiscard]] static Result<std::string> convert(
        std::string_view expression_text,
        const LatexFormatOptions& options = {},
        const std::vector<std::string>& variable_names = {"x", "y", "z", "t", "r", "u", "v", "h"}
    );
};

} // namespace formulaic
