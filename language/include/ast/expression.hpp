#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace lang::ast
{

enum class ExprType
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

enum class KeywordSets
{
    SYSTEM,
    CONTAINER,
    COMPONENT,
    CODE,
    DEPLOY,
    INFRASTRUCTURE
};

template <KeywordSets> struct KeywordExpr;
template <typename> struct LiteralExpr;
struct SetExpr;
struct VariableExpr;
template <ExprType> struct AccessExpr;
struct CallExpr;
template <ExprType> struct UnaryExpr;
template <ExprType> struct MultExpr;
template <ExprType> struct AddExpr;
template <ExprType> struct LogicalExpr;
template <ExprType> struct BooleanExpr;
struct TernaryExpr;

using SystemPtr = std::unique_ptr<KeywordExpr<KeywordSets::SYSTEM>>;
using ContainerPtr = std::unique_ptr<KeywordExpr<KeywordSets::CONTAINER>>;
using ComponentPtr = std::unique_ptr<KeywordExpr<KeywordSets::COMPONENT>>;
using CodePtr = std::unique_ptr<KeywordExpr<KeywordSets::CODE>>;
using DeployPtr = std::unique_ptr<KeywordExpr<KeywordSets::DEPLOY>>;
using InfrastructurePtr = std::unique_ptr<KeywordExpr<KeywordSets::INFRASTRUCTURE>>;
using NumberPtr = std::unique_ptr<LiteralExpr<int64_t>>;
using StringPtr = std::unique_ptr<LiteralExpr<std::string>>;
using BoolPtr = std::unique_ptr<LiteralExpr<bool>>;
using SetPtr = std::unique_ptr<SetExpr>;
using VariablePtr = std::unique_ptr<VariableExpr>;
using CallPtr = std::unique_ptr<CallExpr>;
using AccessExprPtr = std::unique_ptr<AccessExpr<ExprType::ACCESS>>;
using SafeAccessExprPtr = std::unique_ptr<AccessExpr<ExprType::SAFE_ACCESS>>;
using NegationPtr = std::unique_ptr<UnaryExpr<ExprType::NEG>>;
using MultiplyPtr = std::unique_ptr<MultExpr<ExprType::MULT>>;
using DivisionPtr = std::unique_ptr<MultExpr<ExprType::DIV>>;
using AddPtr = std::unique_ptr<AddExpr<ExprType::PLUS>>;
using MinusPtr = std::unique_ptr<AddExpr<ExprType::MINUS>>;
using EqualPtr = std::unique_ptr<BooleanExpr<ExprType::EQ>>;
using NotEqualPtr = std::unique_ptr<BooleanExpr<ExprType::NOT_EQ>>;
using LessPtr = std::unique_ptr<BooleanExpr<ExprType::LESS>>;
using GreaterPtr = std::unique_ptr<BooleanExpr<ExprType::GREATER>>;
using GreateEqualPtr = std::unique_ptr<BooleanExpr<ExprType::GREATER_EQ>>;
using LessEqualPtr = std::unique_ptr<BooleanExpr<ExprType::LESS_EQ>>;
using AndPtr = std::unique_ptr<LogicalExpr<ExprType::AND>>;
using OrPtr = std::unique_ptr<LogicalExpr<ExprType::OR>>;
using XorPtr = std::unique_ptr<LogicalExpr<ExprType::XOR>>;
using InPtr = std::unique_ptr<LogicalExpr<ExprType::IN>>;
using TernaryExprPtr = std::unique_ptr<TernaryExpr>;

using Expression =
    std::variant<SystemPtr, ContainerPtr, ComponentPtr, CodePtr, DeployPtr, InfrastructurePtr,
                 NumberPtr, StringPtr, BoolPtr, SetPtr, VariablePtr, CallPtr, AccessExprPtr,
                 SafeAccessExprPtr, NegationPtr, MultiplyPtr, DivisionPtr, AddPtr, MinusPtr,
                 EqualPtr, NotEqualPtr, LessPtr, GreaterPtr, GreateEqualPtr, LessEqualPtr, AndPtr,
                 OrPtr, XorPtr, InPtr, TernaryExprPtr>;

using ExpressionPtr = std::unique_ptr<Expression>;

template <KeywordSets> struct KeywordExpr
{
};

template <typename T> struct LiteralExpr
{
    T value;
};

struct SetExpr
{
    std::vector<ExpressionPtr> items;
};

struct VariableExpr
{
    std::string name;
};

template <ExprType> struct AccessExpr
{
    ExpressionPtr operand;
    std::string prop;
};

template <ExprType> struct UnaryExpr
{
    ExpressionPtr operand;
};

struct CallExpr
{
    std::string functionName;
    std::vector<ExpressionPtr> args;
};

template <ExprType> struct MultExpr
{
    ExpressionPtr left;
    ExpressionPtr right;
};

template <ExprType> struct AddExpr
{
    ExpressionPtr left;
    ExpressionPtr right;
};

template <ExprType> struct LogicalExpr
{
    ExpressionPtr left;
    ExpressionPtr right;
};

template <ExprType> struct BooleanExpr
{
    ExpressionPtr left;
    ExpressionPtr right;
};

struct TernaryExpr
{
    ExpressionPtr condition;
    ExpressionPtr thenExpr;
    ExpressionPtr elseExpr;
};

} // namespace lang::ast