#pragma once

#include "expression.hpp"

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace lang::ast
{

enum class QuantifierType
{
    ALL,
    ANY
};

enum class CondType
{
    SHORT,
    FULL
};

struct AssignmentStatement;
template <QuantifierType> struct QuantifierStatement;
struct IfThen;
struct IfThenElse;
struct ExceptStatement;
struct StatementExpression;
struct FilteredStatement;

using Source = std::variant<SystemPtr, ContainerPtr, ComponentPtr, CodePtr, DeployPtr,
                            InfrastructurePtr, CallPtr>;
using SourcePtr = std::unique_ptr<Source>;

using AssignmentStatementPtr = std::unique_ptr<AssignmentStatement>;
using AllQuantifierStatementPtr = std::unique_ptr<QuantifierStatement<QuantifierType::ALL>>;
using ExistQuantifierStatementPtr = std::unique_ptr<QuantifierStatement<QuantifierType::ANY>>;
using IfThenPtr = std::unique_ptr<IfThen>;
using IfThenElsePtr = std::unique_ptr<IfThenElse>;
using ExceptStatementPtr = std::unique_ptr<ExceptStatement>;
using StatementExpressionPtr = std::unique_ptr<StatementExpression>;
using FilteredStatementPtr = std::unique_ptr<FilteredStatement>;

using QuantifierPtr = std::variant<AllQuantifierStatementPtr, ExistQuantifierStatementPtr>;
using Сondition = std::variant<IfThenPtr, IfThenElsePtr>;
using СonditionPtr = std::unique_ptr<Сondition>;
using BaseStatement = std::variant<QuantifierPtr, СonditionPtr>;
using BaseStatementPtr = std::unique_ptr<BaseStatement>;
using Predicate = std::variant<StatementExpressionPtr, BaseStatementPtr, FilteredStatementPtr>;
using PredicatePtr = std::unique_ptr<Predicate>;
using BodyStatement = std::variant<QuantifierPtr, ExceptStatementPtr, AssignmentStatementPtr>;
using BodyStatementPtr = std::unique_ptr<BodyStatement>;
using Statement = std::variant<QuantifierPtr, BaseStatementPtr, PredicatePtr, BodyStatementPtr>;
using StatementPtr = std::unique_ptr<Statement>;
using BodyStatementList = std::vector<BodyStatementPtr>;

struct AssignmentStatement
{
    std::string name;
    ExpressionPtr valueExpr;
};

template <QuantifierType T> struct QuantifierStatement
{
    std::vector<std::string> identifiersList;
    ExpressionPtr source;
    PredicatePtr predicate;
};

struct IfThen
{
    ExpressionPtr expr;
    PredicatePtr then;
};

struct IfThenElse
{
    ExpressionPtr expr;
    PredicatePtr then;
    PredicatePtr els;
};

struct ExceptStatement
{
    QuantifierPtr inner;
};

struct StatementExpression
{
    ExpressionPtr expr;
};

struct FilteredStatement
{
    StatementExpressionPtr expr;
    QuantifierPtr quant;
};

} // namespace lang::ast