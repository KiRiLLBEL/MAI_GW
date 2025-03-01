#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace lang::ast
{

enum class ExprOpType
{
    PLUS,
    MINUS,
    MULT,
    DIV,
    NEG,
    EQ,
    NOT_EQ,
    LESS,
    GREATER,
    LESS_EQ,
    GREATER_EQ,
    IN,
    AND,
    OR,
    XOR,
    ACCESS,
    SAFE_ACCESS
};

struct Literal;
struct Variable;
struct TypeCheck;
struct BinaryExpr;
struct AccessExpr;
struct UnaryExpr;
struct TernaryExpr;
struct FunctionCall;

using LiteralPtr = std::unique_ptr<Literal>;
using VariablePtr = std::unique_ptr<Variable>;
using TypeCheckPtr = std::unique_ptr<TypeCheck>;
using BinaryExprPtr = std::unique_ptr<BinaryExpr>;
using UnaryExpPtr = std::unique_ptr<UnaryExpr>;
using AccessExprPtr = std::unique_ptr<AccessExpr>;
using TernaryExprPtr = std::unique_ptr<TernaryExpr>;
using FunctionCallPtr = std::unique_ptr<FunctionCall>;
    

using Expression = std::variant
<
    LiteralPtr,
    VariablePtr,
    TypeCheckPtr,
    BinaryExprPtr,
    UnaryExpPtr,
    AccessExprPtr,
    TernaryExprPtr,
    FunctionCallPtr
>;

using ExpressionPtr = std::unique_ptr<Expression>;

enum class LiteralType
{
    String,
    Number,
    Bool,
    Set
};

struct Literal
{
    LiteralType litType;

    std::variant
    <
        std::string,
        int64_t,
        bool,
        std::vector<ExpressionPtr>
    > value;
};

struct Variable
{
    std::string name;
};

struct TypeCheck
{
    ExpressionPtr expr;
    std::string typeName;
};

struct AccessExpr
{
    ExpressionPtr operand;
    ExprOpType op;
    std::string prop;
};

struct UnaryExpr
{
    ExprOpType op;
    ExpressionPtr operand;
};

struct BinaryExpr
{
    ExpressionPtr left;
    ExprOpType op;
    ExpressionPtr right;
};

struct TernaryExpr
{
    ExpressionPtr condition;
    ExpressionPtr thenExpr;
    ExpressionPtr elseExpr;
};

struct FunctionCall
{
    std::string functionName;
    std::vector<ExpressionPtr> args;
};

}