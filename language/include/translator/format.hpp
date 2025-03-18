#pragma once

#include <array>
#include <lang/expression.hpp>
#include <string>
#include <unordered_map>

namespace cypher
{
using namespace std::literals::string_view_literals;
static constexpr auto ruleNameFormat = "// [RULE]: {}"sv;
static constexpr auto descriptionFormat = "// [DESCRIPTION]: {}"sv;
static constexpr auto priorityFormat = "// [PRIORITY]: {}"sv;
static constexpr auto inDevelopmentFormat = "// [NOW IN DEVELOPMENT]"sv;
static constexpr auto whereFormat = "WHERE {}"sv;
static constexpr auto quantFormat = "{}({} IN {} WHERE {})"sv;
static constexpr auto matchFormat = "MATCH (obj:{})"sv;
static constexpr auto withCollectFormat = "WITH collect(obj) AS {}"sv;
static constexpr auto withVariableFormat = "WITH {} AS {}"sv;

constexpr auto OperatorMap(const lang::ast::ExprOpType type)
{
    switch (type)
    {
    case lang::ast::ExprOpType::PLUS:
        return "{} + {}"sv;
    case lang::ast::ExprOpType::MINUS:
        return "{} - {}"sv;
    case lang::ast::ExprOpType::MULT:
        return "{} * {}"sv;
    case lang::ast::ExprOpType::DIV:
        return "{} / {}"sv;
    case lang::ast::ExprOpType::NEG:
        return "-{}"sv;
    case lang::ast::ExprOpType::EQ:
        return "{} = {}"sv;
    case lang::ast::ExprOpType::NOT_EQ:
        return "{} <> {}"sv;
    case lang::ast::ExprOpType::LESS:
        return "{} < {}"sv;
    case lang::ast::ExprOpType::GREATER:
        return "{} > {}"sv;
    case lang::ast::ExprOpType::LESS_EQ:
        return "{} <= {}"sv;
    case lang::ast::ExprOpType::GREATER_EQ:
        return "{} >= {}"sv;
    case lang::ast::ExprOpType::IN:
        return "{} IN {}"sv;
    case lang::ast::ExprOpType::AND:
        return "{} AND {}"sv;
    case lang::ast::ExprOpType::OR:
        return "{} OR {}"sv;
    case lang::ast::ExprOpType::XOR:
        return "{} XOR {}"sv;
    case lang::ast::ExprOpType::ACCESS:
        return "{}.{}"sv;
    case lang::ast::ExprOpType::SAFE_ACCESS:
        return "exists({}.{})"sv;
    default:
        return ""sv;
    }
}

static constexpr auto superSetsContainsError = "Variable [{}] not in super set"sv;
static constexpr auto selectionFromRouteError = "Selection must be from route function"sv;
} // namespace cypher