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
static constexpr auto matchFormat = "MATCH (obj__:{})"sv;
static constexpr auto withCollectFormat = "WITH collect(obj__) AS {}"sv;
static constexpr auto withVariableFormat = "WITH {} AS {}"sv;
static constexpr auto withSelectionFormat = "WITH "sv;
static constexpr auto matchLiteralFormat = "MATCH ({})-->({}) "sv;
static constexpr auto routeExistFormat = "MATCH route__=({})-[*]->(mid__)-[*]->({})"sv;
static constexpr auto routeAllFormat = "MATCH route__=({})-[*]->({})"sv;
static constexpr auto callFormat = "CALL {{ {}\n{} }}"sv;
constexpr auto OperatorMap(const lang::ast::ExprType type)
{
    switch (type)
    {
    case lang::ast::ExprType::PLUS:
        return "{} + {}"sv;
    case lang::ast::ExprType::MINUS:
        return "{} - {}"sv;
    case lang::ast::ExprType::MULT:
        return "{} * {}"sv;
    case lang::ast::ExprType::DIV:
        return "{} / {}"sv;
    case lang::ast::ExprType::NEG:
        return "-{}"sv;
    case lang::ast::ExprType::EQ:
        return "{} = {}"sv;
    case lang::ast::ExprType::NOT_EQ:
        return "{} <> {}"sv;
    case lang::ast::ExprType::LESS:
        return "{} < {}"sv;
    case lang::ast::ExprType::GREATER:
        return "{} > {}"sv;
    case lang::ast::ExprType::LESS_EQ:
        return "{} <= {}"sv;
    case lang::ast::ExprType::GREATER_EQ:
        return "{} >= {}"sv;
    case lang::ast::ExprType::IN:
        return "{} IN {}"sv;
    case lang::ast::ExprType::AND:
        return "{} AND {}"sv;
    case lang::ast::ExprType::OR:
        return "{} OR {}"sv;
    case lang::ast::ExprType::XOR:
        return "{} XOR {}"sv;
    case lang::ast::ExprType::ACCESS:
        return "{}.{}"sv;
    case lang::ast::ExprType::SAFE_ACCESS:
        return "exists({}.{})"sv;
    default:
        return ""sv;
    }
}

static constexpr auto superSetsContainsError = "Variable [{}] not in super set"sv;
static constexpr auto selectionFromRouteError = "Selection must be from route function"sv;
} // namespace cypher