#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <lang/expression.hpp>

namespace cypher {
    using namespace std::literals::string_view_literals;
    static constexpr auto RuleNameFormat = "// [RULE]: {}"sv;
    static constexpr auto DescriptionFormat = "// [DESCRIPTION]: {}"sv;
    static constexpr auto PriorityFormat = "// [PRIORITY]: {}"sv;
    static constexpr auto InDevelopmentFormat = "// [NOW IN DEVELOPMENT]"sv;
    static constexpr auto WhereFormat = "WHERE {}"sv;
    static constexpr auto QuantFormat = "{}({} IN {} WHERE {})"sv;
    static constexpr auto MatchFormat = "MATCH (obj:{})"sv;
    static constexpr auto WithCollectFormat = "WITH collect(obj) AS {}"sv;
    static constexpr auto WithVariableFormat = "WITH {} AS {}"sv;

constexpr auto OperatorMap(const lang::ast::ExprOpType type)
{
    switch (type)
    {
        case lang::ast::ExprOpType::PLUS:        return "{} + {}"sv;
        case lang::ast::ExprOpType::MINUS:       return "{} - {}"sv;
        case lang::ast::ExprOpType::MULT:        return "{} * {}"sv;
        case lang::ast::ExprOpType::DIV:         return "{} / {}"sv;
        case lang::ast::ExprOpType::NEG:         return "-{}"sv;
        case lang::ast::ExprOpType::EQ:          return "{} = {}"sv;
        case lang::ast::ExprOpType::NOT_EQ:      return "{} <> {}"sv;
        case lang::ast::ExprOpType::LESS:        return "{} < {}"sv;
        case lang::ast::ExprOpType::GREATER:     return "{} > {}"sv;
        case lang::ast::ExprOpType::LESS_EQ:     return "{} <= {}"sv;
        case lang::ast::ExprOpType::GREATER_EQ:  return "{} >= {}"sv;
        case lang::ast::ExprOpType::IN:          return "{} IN {}"sv;
        case lang::ast::ExprOpType::AND:         return "{} AND {}"sv;
        case lang::ast::ExprOpType::OR:          return "{} OR {}"sv;
        case lang::ast::ExprOpType::XOR:         return "{} XOR {}"sv;
        case lang::ast::ExprOpType::ACCESS:      return "{}.{}"sv;
        case lang::ast::ExprOpType::SAFE_ACCESS: return "exists({}.{})"sv;
        default:                                 return ""sv;
    }
}


    static constexpr auto SuperSetsContainsError = "Variable [{}] not in super set"sv;
    static constexpr auto SelectionFromRouteError = "Selection must be from route function"sv; 
}