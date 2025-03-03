#pragma once

#include "expression.hpp"

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

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
struct СonditionalStatement;

using AssignmentStatementPtr = std::unique_ptr<AssignmentStatement>;
using QuantifierStatementPtr = std::unique_ptr<QuantifierStatement>;
using SelectionStatementPtr = std::unique_ptr<SelectionStatement>;
using СonditionalStatementPtr = std::unique_ptr<СonditionalStatement>;

using Statement = std::variant
<
    AssignmentStatementPtr,
    QuantifierStatementPtr,
    SelectionStatementPtr,
    СonditionalStatementPtr
>;

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
    All,
    Exist
};

struct QuantifierStatement
{
    QuantifierType type;
    StatementPtr body;
};

struct SelectionStatement
{
    std::vector<std::string> varNames;
    ExpressionPtr collectionExpr;
    StatementPtr body;
};

struct СonditionalStatement
{
    ExpressionPtr collectionExpr;
    StatementPtr then;
    std::optional<StatementPtr> body;
};

struct Rule
{
    std::string name;
    std::optional<std::string> description;
    Priority priority{Priority::ERROR};
    BlockPtr mainBlock;
    std::vector<BlockPtr> exceptBlocks;
};

}