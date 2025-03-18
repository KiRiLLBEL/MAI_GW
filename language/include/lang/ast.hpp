#pragma once

#include "expression.hpp"

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace lang::ast
{

enum class Priority
{
    INFO,
    WARN,
    ERROR
};

struct AssignmentStatement;
struct QuantifierStatement;
struct SelectionStatement;
struct ConditionalStatement;
struct ExceptStatement;

using AssignmentStatementPtr = std::unique_ptr<AssignmentStatement>;
using QuantifierStatementPtr = std::unique_ptr<QuantifierStatement>;
using SelectionStatementPtr = std::unique_ptr<SelectionStatement>;
using СonditionalStatementPtr = std::unique_ptr<ConditionalStatement>;
using ExceptStatementPtr = std::unique_ptr<ExceptStatement>;

using Statement = std::variant<AssignmentStatementPtr, QuantifierStatementPtr,
                               SelectionStatementPtr, СonditionalStatementPtr, ExceptStatementPtr>;

using StatementPtr = std::unique_ptr<Statement>;

struct Block
{
    std::vector<StatementPtr> statements;
};

using BlockPtr = std::unique_ptr<Block>;

struct AssignmentStatement
{
    std::string name;
    ExpressionPtr valueExpr;
};

enum class QuantifierType
{
    ALL,
    ANY
};

struct QuantifierStatement
{
    QuantifierType type;
    SelectionStatementPtr body;
};

struct SelectionStatement
{
    std::vector<std::string> varNames;
    ExpressionPtr collectionExpr;
    std::variant<StatementPtr, ExpressionPtr> body;
    std::optional<StatementPtr> quant;
};

struct ConditionalStatement
{
    ExpressionPtr collectionExpr;
    StatementPtr then;
    std::optional<StatementPtr> body;
};

struct ExceptStatement
{
    StatementPtr inner;
};

struct Rule
{
    std::string name;
    std::optional<std::string> description;
    Priority priority{Priority::ERROR};
    BlockPtr calls;
};

} // namespace lang::ast