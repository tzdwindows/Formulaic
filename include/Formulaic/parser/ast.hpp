#pragma once

#include <Formulaic/core/error.hpp>
#include <Formulaic/parser/token.hpp>
#include <memory>
#include <string>
#include <vector>

namespace formulaic {

class ASTNode {
public:
    virtual ~ASTNode() = default;
    SourceLocation location;
};

class NumberNode final : public ASTNode {
public:
    double value{0.0};
    explicit NumberNode(double val, SourceLocation loc = {}) : value(val) {
        location = loc;
    }
};

class VariableNode final : public ASTNode {
public:
    std::string name;
    explicit VariableNode(std::string var_name, SourceLocation loc = {})
        : name(std::move(var_name)) {
        location = loc;
    }
};

enum class UnaryOp {
    Negate,
    Not
};

class UnaryOpNode final : public ASTNode {
public:
    UnaryOp op{UnaryOp::Negate};
    std::unique_ptr<ASTNode> operand;

    UnaryOpNode(UnaryOp o, std::unique_ptr<ASTNode> expr, SourceLocation loc = {})
        : op(o), operand(std::move(expr)) {
        location = loc;
    }
};

enum class BinaryOp {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Power,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    LogicalAnd,
    LogicalOr
};

class BinaryOpNode final : public ASTNode {
public:
    BinaryOp op{BinaryOp::Add};
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;

    BinaryOpNode(BinaryOp o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r, SourceLocation loc = {})
        : op(o), left(std::move(l)), right(std::move(r)) {
        location = loc;
    }
};

class FunctionCallNode final : public ASTNode {
public:
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> args;

    FunctionCallNode(std::string fn_name, std::vector<std::unique_ptr<ASTNode>> arguments, SourceLocation loc = {})
        : name(std::move(fn_name)), args(std::move(arguments)) {
        location = loc;
    }
};

class VarDeclNode final : public ASTNode {
public:
    std::string name;
    std::unique_ptr<ASTNode> initializer;

    VarDeclNode(std::string var_name, std::unique_ptr<ASTNode> init_expr, SourceLocation loc = {})
        : name(std::move(var_name)), initializer(std::move(init_expr)) {
        location = loc;
    }
};

class AssignmentNode final : public ASTNode {
public:
    std::string name;
    std::unique_ptr<ASTNode> value;

    AssignmentNode(std::string var_name, std::unique_ptr<ASTNode> val_expr, SourceLocation loc = {})
        : name(std::move(var_name)), value(std::move(val_expr)) {
        location = loc;
    }
};

class BlockNode final : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    std::unique_ptr<ASTNode> result_expr;

    BlockNode(std::vector<std::unique_ptr<ASTNode>> stmts, std::unique_ptr<ASTNode> result, SourceLocation loc = {})
        : statements(std::move(stmts)), result_expr(std::move(result)) {
        location = loc;
    }
};

} // namespace formulaic
